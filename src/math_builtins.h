#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace UCLang {
struct Value;
void register_math_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& map);
} // namespace UCLang
