#include "asset_builtins.h"
#include "interpreter.h"
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <stdexcept>

namespace UCLang {

// ─── GUID generation ───
static std::string generateGUID() {
    static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    static std::uniform_int_distribution<uint64_t> dist;
    char buf[37];
    std::sprintf(buf, "%08x-%04x-%04x-%04x-%012llx",
        (unsigned)(dist(rng) & 0xFFFFFFFF),
        (unsigned)(dist(rng) & 0xFFFF),
        (unsigned)((dist(rng) & 0x0FFF) | 0x4000),
        (unsigned)((dist(rng) & 0x3FFF) | 0x8000),
        (unsigned long long)(dist(rng) & 0xFFFFFFFFFFFFULL));
    return std::string(buf);
}

// ─── Asset registry ───
struct AssetEntry {
    std::string guid;
    std::string path;      // original path
    std::string type;      // "mesh", "texture", "shader", "font", "script"
    std::string resolved;  // current resolved path (after rename)
    int64_t lastModified;
};

static std::map<std::string, AssetEntry> g_guidToAsset;  // guid -> asset
static std::map<std::string, std::string> g_pathToGuid;  // path -> guid
static std::atomic<bool> g_hotReloadRunning{false};
static std::thread g_watcherThread;
static std::vector<std::string> g_watchDirs;
static int64_t g_nextAssetId = 1000;

static int64_t fileTime(const std::string& path) {
    try {
        auto ft = std::filesystem::last_write_time(path);
        return std::chrono::duration_cast<std::chrono::seconds>(
            ft.time_since_epoch()).count();
    } catch (...) { return 0; }
}

// ─── Watcher thread ───
static std::vector<std::string> g_hotReloadedAssets;
static std::mutex g_hotReloadMutex;

static void watcherLoop() {
    // Store last known modified times
    std::map<std::string, int64_t> lastTimes;
    for (const auto& [guid, entry] : g_guidToAsset)
        lastTimes[entry.resolved] = entry.lastModified;

    while (g_hotReloadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> lock(g_hotReloadMutex);
        for (auto& [guid, entry] : g_guidToAsset) {
            int64_t ft = fileTime(entry.resolved);
            if (ft > 0 && ft != entry.lastModified) {
                entry.lastModified = ft;
                g_hotReloadedAssets.push_back(guid);
            }
        }
    }
}

void register_asset_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m) {
    // asset_register(path, type) -> guid
    // Registers an asset, returns its GUID. If path was already registered, returns existing GUID.
    m["asset_register"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("asset_register(path,type): need 2");
        auto* path = std::get_if<std::string>(&args[0]);
        auto* type = std::get_if<std::string>(&args[1]);
        if (!path || !type) throw std::runtime_error("asset_register: path and type must be strings");

        // Check if already registered
        auto pit = g_pathToGuid.find(*path);
        if (pit != g_pathToGuid.end()) {
            return Value(pit->second);
        }

        std::string guid = generateGUID();
        AssetEntry entry;
        entry.guid = guid;
        entry.path = *path;
        entry.type = *type;
        entry.resolved = *path;
        entry.lastModified = fileTime(*path);
        g_guidToAsset[guid] = entry;
        g_pathToGuid[*path] = guid;
        return Value(guid);
    };

    // asset_guid_to_path(guid) -> path
    m["asset_guid_to_path"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_guid_to_path(guid): need guid");
        auto* guid = std::get_if<std::string>(&args[0]);
        if (!guid) throw std::runtime_error("asset_guid_to_path: guid must be string");
        auto it = g_guidToAsset.find(*guid);
        if (it == g_guidToAsset.end()) return Value(std::string(""));
        return Value(it->second.resolved);
    };

    // asset_path_to_guid(path) -> guid
    m["asset_path_to_guid"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_path_to_guid(path): need path");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("asset_path_to_guid: path must be string");
        auto it = g_pathToGuid.find(*path);
        if (it == g_pathToGuid.end()) return Value(std::string(""));
        return Value(it->second);
    };

    // asset_rename(old_path, new_path) — updates registry when an asset is renamed
    m["asset_rename"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("asset_rename(old_path,new_path): need 2");
        auto* oldPath = std::get_if<std::string>(&args[0]);
        auto* newPath = std::get_if<std::string>(&args[1]);
        if (!oldPath || !newPath) throw std::runtime_error("asset_rename: paths must be strings");
        auto pit = g_pathToGuid.find(*oldPath);
        if (pit == g_pathToGuid.end()) return Value(false);
        std::string guid = pit->second;
        auto ait = g_guidToAsset.find(guid);
        if (ait != g_guidToAsset.end()) {
            ait->second.path = *newPath;
            ait->second.resolved = *newPath;
            ait->second.lastModified = fileTime(*newPath);
        }
        g_pathToGuid.erase(*oldPath);
        g_pathToGuid[*newPath] = guid;
        return Value(true);
    };

    // asset_get_type(guid) -> type string
    m["asset_get_type"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_get_type(guid): need guid");
        auto* guid = std::get_if<std::string>(&args[0]);
        if (!guid) throw std::runtime_error("asset_get_type: guid must be string");
        auto it = g_guidToAsset.find(*guid);
        if (it == g_guidToAsset.end()) return Value(std::string(""));
        return Value(it->second.type);
    };

    // asset_is_dirty(guid) -> 1 if asset changed since last check, 0 otherwise
    m["asset_is_dirty"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_is_dirty(guid): need guid");
        auto* guid = std::get_if<std::string>(&args[0]);
        if (!guid) throw std::runtime_error("asset_is_dirty: guid must be string");
        std::lock_guard<std::mutex> lock(g_hotReloadMutex);
        auto it = std::find(g_hotReloadedAssets.begin(), g_hotReloadedAssets.end(), *guid);
        if (it != g_hotReloadedAssets.end()) {
            g_hotReloadedAssets.erase(it);
            return Value((int64_t)1);
        }
        return Value((int64_t)0);
    };

    // asset_check_all() -> list of dirty GUIDs
    m["asset_check_all"] = [](const std::vector<Value>&) -> Value {
        std::lock_guard<std::mutex> lock(g_hotReloadMutex);
        ValueList result;
        for (const auto& g : g_hotReloadedAssets)
            result.push_back(Value(g));
        g_hotReloadedAssets.clear();
        return Value(result);
    };

    // asset_watch_start(dir) — starts a background thread watching all registered assets
    m["asset_watch_start"] = [](const std::vector<Value>&) -> Value {
        if (g_hotReloadRunning) return std::monostate{};
        g_hotReloadRunning = true;
        g_watcherThread = std::thread(watcherLoop);
        g_watcherThread.detach();
        return std::monostate{};
    };

    // asset_watch_stop() — stops the watcher thread
    m["asset_watch_stop"] = [](const std::vector<Value>&) -> Value {
        g_hotReloadRunning = false;
        return std::monostate{};
    };

    // asset_save_registry(path) — saves GUID map to disk
    m["asset_save_registry"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_save_registry(path): need path");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("asset_save_registry: path must be string");
        std::ofstream f(*path);
        if (!f.is_open()) throw std::runtime_error("Cannot open " + *path);
        f << "{\n";
        bool first = true;
        for (const auto& [guid, entry] : g_guidToAsset) {
            if (!first) f << ",\n";
            first = false;
            f << "  \"" << guid << "\": {\n";
            f << "    \"path\": \"" << entry.path << "\",\n";
            f << "    \"type\": \"" << entry.type << "\",\n";
            f << "    \"resolved\": \"" << entry.resolved << "\"\n";
            f << "  }";
        }
        f << "\n}\n";
        return std::monostate{};
    };

    // asset_load_registry(path) — loads GUID map from disk
    m["asset_load_registry"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("asset_load_registry(path): need path");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("asset_load_registry: path must be string");
        std::ifstream f(*path);
        if (!f.is_open()) return Value(false);
        // Simple parser of the registry format
        std::stringstream ss;
        ss << f.rdbuf();
        std::string json = ss.str();
        // Find each GUID entry
        size_t pos = 0;
        while (pos < json.size()) {
            // Find "xxxxxxxx-...-xxxx": {
            size_t gs = json.find('"', pos);
            if (gs == std::string::npos) break;
            size_t ge = json.find('"', gs + 1);
            if (ge == std::string::npos) break;
            std::string guid = json.substr(gs + 1, ge - gs - 1);
            pos = ge + 1;
            // Find path, type, resolved
            auto findField = [&](const std::string& field) -> std::string {
                std::string search = "\"" + field + "\": \"";
                size_t p = json.find(search, pos);
                if (p == std::string::npos) return "";
                p += search.size();
                size_t e = json.find('"', p);
                if (e == std::string::npos) return "";
                return json.substr(p, e - p);
            };
            std::string pathVal = findField("path");
            std::string typeVal = findField("type");
            std::string resolvedVal = findField("resolved");
            if (!guid.empty() && !pathVal.empty()) {
                AssetEntry entry;
                entry.guid = guid;
                entry.path = pathVal;
                entry.type = typeVal.empty() ? "unknown" : typeVal;
                entry.resolved = resolvedVal.empty() ? pathVal : resolvedVal;
                entry.lastModified = fileTime(entry.resolved);
                g_guidToAsset[guid] = entry;
                g_pathToGuid[entry.path] = guid;
            }
        }
        return Value(true);
    };

    // asset_list() -> [[guid, path, type], ...]
    m["asset_list"] = [](const std::vector<Value>&) -> Value {
        ValueList result;
        for (const auto& [guid, entry] : g_guidToAsset) {
            ValueList row;
            row.push_back(Value(guid));
            row.push_back(Value(entry.path));
            row.push_back(Value(entry.type));
            result.push_back(Value(row));
        }
        return Value(result);
    };
}

} // namespace UCLang