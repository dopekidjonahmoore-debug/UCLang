#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "ast.h"
#include "interpreter.h"

namespace UCLang {
void register_asset_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m);
} // namespace UCLang