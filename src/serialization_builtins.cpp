#include "serialization_builtins.h"
#include "interpreter.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace UCLang {

static std::string valueToJson(const Value& v, bool pretty, int indent) {
    std::string tabs;
    if (pretty) for (int i = 0; i < indent; ++i) tabs += "  ";
    std::string sep = pretty ? ",\n" : ",";

    if (std::holds_alternative<std::monostate>(v)) return "null";
    if (auto* d = std::get_if<double>(&v)) {
        std::ostringstream os;
        if (std::floor(*d) == *d && !std::isinf(*d) && !std::isnan(*d))
            os << (int64_t)*d;
        else
            os << *d;
        return os.str();
    }
    if (auto* i = std::get_if<int64_t>(&v)) return std::to_string(*i);
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    if (auto* s = std::get_if<std::string>(&v)) {
        std::string esc;
        for (char c : *s) {
            if (c == '"') esc += "\\\"";
            else if (c == '\\') esc += "\\\\";
            else if (c == '\n') esc += "\\n";
            else if (c == '\r') esc += "\\r";
            else if (c == '\t') esc += "\\t";
            else esc += c;
        }
        return "\"" + esc + "\"";
    }
    if (auto* list = std::get_if<ValueList>(&v)) {
        if (list->empty()) return "[]";
        std::string json = "[";
        if (pretty) json += "\n";
        for (size_t i = 0; i < list->size(); ++i) {
            if (pretty) json += tabs + "  ";
            json += valueToJson((*list)[i], pretty, indent + 1);
            if (i + 1 < list->size()) json += sep;
            else if (pretty) json += "\n";
        }
        if (pretty) json += tabs;
        json += "]";
        return json;
    }
    if (auto* obj = std::get_if<std::shared_ptr<UCLangObjectData>>(&v)) {
        std::string json = "{";
        if (pretty) json += "\n";
        bool first = true;
        for (const auto& f : (*obj)->fields) {
            if (!first) json += sep;
            else first = false;
            if (pretty) json += tabs + "  ";
            json += "\"" + f.first + "\":" + (pretty ? " " : "") + valueToJson(f.second, pretty, indent + 1);
        }
        if (pretty && !(*obj)->fields.empty()) json += "\n" + tabs;
        json += "}";
        return json;
    }
    if (std::get_if<Vec3>(&v) || std::get_if<Quat>(&v) || std::get_if<Mat4>(&v))
        return "\"<native>\"";
    return "null";
}

static Value jsonToValue(const std::string& json, size_t& pos) {
    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
    if (pos >= json.size()) return std::monostate{};

    if (json[pos] == '"') {
        pos++; // skip opening quote
        std::string s;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\') {
                pos++;
                if (pos < json.size()) {
                    switch (json[pos]) {
                        case '"': s += '"'; break;
                        case '\\': s += '\\'; break;
                        case 'n': s += '\n'; break;
                        case 'r': s += '\r'; break;
                        case 't': s += '\t'; break;
                        default: s += json[pos];
                    }
                }
            } else {
                s += json[pos];
            }
            pos++;
        }
        if (pos < json.size()) pos++; // skip closing quote
        return Value(s);
    }

    if (json.substr(pos, 4) == "true") { pos += 4; return Value(true); }
    if (json.substr(pos, 5) == "false") { pos += 5; return Value(false); }
    if (json.substr(pos, 4) == "null") { pos += 4; return std::monostate{}; }

    // Number
    if (json[pos] == '-' || (json[pos] >= '0' && json[pos] <= '9')) {
        size_t start = pos;
        if (json[pos] == '-') pos++;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
        bool isFloat = false;
        if (pos < json.size() && json[pos] == '.') {
            isFloat = true;
            pos++;
            while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
        }
        if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
            isFloat = true;
            pos++;
            if (pos < json.size() && (json[pos] == '+' || json[pos] == '-')) pos++;
            while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
        }
        std::string num = json.substr(start, pos - start);
        if (isFloat) return Value(std::stod(num));
        return Value((int64_t)std::stoll(num));
    }

    if (json[pos] == '[') {
        pos++; // skip '['
        ValueList list;
        while (pos < json.size() && json[pos] != ']') {
            list.push_back(jsonToValue(json, pos));
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == ',')) pos++;
        }
        if (pos < json.size()) pos++; // skip ']'
        return Value(list);
    }

    if (json[pos] == '{') {
        pos++; // skip '{'
        auto objData = std::make_shared<UCLangObjectData>();
        objData->className = "JsonObject";
        while (pos < json.size() && json[pos] != '}') {
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
            if (pos >= json.size() || json[pos] == '}') break;
            // Expect string key
            if (json[pos] != '"') { pos++; continue; }
            Value keyVal = jsonToValue(json, pos);
            std::string key = std::get_if<std::string>(&keyVal) ? *std::get_if<std::string>(&keyVal) : "";
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == ':')) pos++;
            Value val = jsonToValue(json, pos);
            objData->fields[key] = val;
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == ',')) pos++;
        }
        if (pos < json.size()) pos++; // skip '}'
        return Value(objData);
    }

    return std::monostate{};
}

void register_serialization_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m) {
    m["serialize.to_json"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.to_json(value, pretty?): need at least 1 arg");
        bool pretty = args.size() > 1 && std::get_if<bool>(&args[1]) && *std::get_if<bool>(&args[1]);
        return Value(valueToJson(args[0], pretty, 0));
    };

    m["serialize.from_json"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.from_json(json_string): need 1 arg");
        auto* str = std::get_if<std::string>(&args[0]);
        if (!str) throw std::runtime_error("serialize.from_json: argument must be string");
        size_t pos = 0;
        return jsonToValue(*str, pos);
    };

    m["serialize.save_json"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("serialize.save_json(path, value, pretty?): need at least 2 args");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("serialize.save_json: path must be string");
        bool pretty = args.size() > 2 && std::get_if<bool>(&args[2]) && *std::get_if<bool>(&args[2]);
        std::string json = valueToJson(args[1], pretty, 0);
        std::ofstream f(*path);
        if (!f.is_open()) throw std::runtime_error("serialize.save_json: cannot open file: " + *path);
        f << json;
        return Value(true);
    };

    m["serialize.load_json"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.load_json(path): need 1 arg");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("serialize.load_json: path must be string");
        std::ifstream f(*path);
        if (!f.is_open()) throw std::runtime_error("serialize.load_json: cannot open file: " + *path);
        std::stringstream ss;
        ss << f.rdbuf();
        size_t pos = 0;
        return jsonToValue(ss.str(), pos);
    };

    m["serialize.save_text"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("serialize.save_text(path, text): need 2 args");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("serialize.save_text: path must be string");
        auto* text = std::get_if<std::string>(&args[1]);
        if (!text) throw std::runtime_error("serialize.save_text: text must be string");
        std::ofstream f(*path);
        if (!f.is_open()) throw std::runtime_error("serialize.save_text: cannot open file: " + *path);
        f << *text;
        return Value(true);
    };

    m["serialize.load_text"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.load_text(path): need 1 arg");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("serialize.load_text: path must be string");
        std::ifstream f(*path);
        if (!f.is_open()) throw std::runtime_error("serialize.load_text: cannot open file: " + *path);
        std::stringstream ss;
        ss << f.rdbuf();
        return Value(ss.str());
    };

    m["serialize.list_files"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.list_files(path): need 1 arg");
        auto* dir = std::get_if<std::string>(&args[0]);
        if (!dir) throw std::runtime_error("serialize.list_files: path must be string");
        ValueList result;
#ifdef _WIN32
        std::string pattern = *dir + "\\*";
        WIN32_FIND_DATAA ffd;
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                        ValueList entry;
                        entry.push_back(Value(std::string(ffd.cFileName)));
                        entry.push_back(Value(std::string("dir")));
                        entry.push_back(Value((int64_t)ffd.nFileSizeLow));
                        result.push_back(Value(entry));
                    }
                } else {
                    ValueList entry;
                    entry.push_back(Value(std::string(ffd.cFileName)));
                    entry.push_back(Value(std::string("file")));
                    entry.push_back(Value((int64_t)ffd.nFileSizeLow));
                    result.push_back(Value(entry));
                }
            } while (FindNextFileA(hFind, &ffd) != 0);
            FindClose(hFind);
        }
#endif
        return Value(result);
    };

    m["serialize.file_exists"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("serialize.file_exists(path): need 1 arg");
        auto* path = std::get_if<std::string>(&args[0]);
        if (!path) throw std::runtime_error("serialize.file_exists: path must be string");
        std::ifstream f(*path);
        return Value((bool)f.is_open());
    };

    m["serialize.copy_file"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("serialize.copy_file(src, dst): need 2 args");
        auto* src = std::get_if<std::string>(&args[0]);
        auto* dst = std::get_if<std::string>(&args[1]);
        if (!src || !dst) throw std::runtime_error("serialize.copy_file: paths must be strings");
        std::ifstream in(*src, std::ios::binary);
        if (!in.is_open()) return Value(false);
        std::ofstream out(*dst, std::ios::binary);
        if (!out.is_open()) return Value(false);
        out << in.rdbuf();
        return Value(true);
    };
}

} // namespace UCLang