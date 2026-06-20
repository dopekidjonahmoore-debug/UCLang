#include "entity_builtins.h"
#include "interpreter.h"
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

namespace UCLang {

static std::unordered_map<int64_t, std::unordered_map<std::string, ValueList>> g_entities;
static int64_t g_nextEntityId = 1;

void register_entity_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& map) {

    map["entity_create"] = [](const std::vector<Value>&) -> Value {
        int64_t id = g_nextEntityId++;
        g_entities[id] = {};
        return Value(id);
    };

    map["entity_destroy"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<int64_t>(&args[0]))
            throw std::runtime_error("entity_destroy(id): need entity ID");
        g_entities.erase(std::get<int64_t>(args[0]));
        return std::monostate{};
    };

    map["entity_exists"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty() || !std::get_if<int64_t>(&args[0]))
            throw std::runtime_error("entity_exists(id): need entity ID");
        return Value(g_entities.find(std::get<int64_t>(args[0])) != g_entities.end());
    };

    map["entity_set"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("entity_set(id, tag, ...): need id and tag");
        auto* idPtr = std::get_if<int64_t>(&args[0]);
        auto* tagPtr = std::get_if<std::string>(&args[1]);
        if (!idPtr || !tagPtr)
            throw std::runtime_error("entity_set: id must be int, tag must be string");
        int64_t id = *idPtr;
        const std::string& tag = *tagPtr;
        ValueList component;
        component.reserve(args.size() - 2);
        for (size_t i = 2; i < args.size(); i++)
            component.push_back(args[i]);
        g_entities[id][tag] = std::move(component);
        return std::monostate{};
    };

    map["entity_get"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("entity_get(id, tag): need id and tag");
        auto* idPtr = std::get_if<int64_t>(&args[0]);
        auto* tagPtr = std::get_if<std::string>(&args[1]);
        if (!idPtr || !tagPtr)
            throw std::runtime_error("entity_get: id must be int, tag must be string");
        int64_t id = *idPtr;
        const std::string& tag = *tagPtr;
        auto it = g_entities.find(id);
        if (it == g_entities.end())
            throw std::runtime_error("entity_get: entity not found");
        auto ct = it->second.find(tag);
        if (ct == it->second.end())
            throw std::runtime_error("entity_get: component " + tag + " not found");
        return Value(ct->second);
    };

    map["entity_has"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("entity_has(id, tag): need id and tag");
        auto* idPtr = std::get_if<int64_t>(&args[0]);
        auto* tagPtr = std::get_if<std::string>(&args[1]);
        if (!idPtr || !tagPtr)
            throw std::runtime_error("entity_has: id must be int, tag must be string");
        int64_t id = *idPtr;
        const std::string& tag = *tagPtr;
        auto it = g_entities.find(id);
        if (it == g_entities.end()) return Value(false);
        return Value(it->second.find(tag) != it->second.end());
    };

    map["entity_all"] = [](const std::vector<Value>&) -> Value {
        ValueList ids;
        ids.reserve(g_entities.size());
        for (const auto& pair : g_entities)
            ids.push_back(Value(pair.first));
        return Value(ids);
    };

    map["entity_count"] = [](const std::vector<Value>&) -> Value {
        return Value(static_cast<int64_t>(g_entities.size()));
    };

    map["entity_clear"] = [](const std::vector<Value>&) -> Value {
        g_entities.clear();
        g_nextEntityId = 1;
        return std::monostate{};
    };
}

} // namespace UCLang
