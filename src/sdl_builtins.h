#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace UCLang {
struct Value;
void run_sdl_game_loop(const std::function<void()>& updateFn, const std::function<void()>& drawFn);
void register_sdl_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& map);
} // namespace UCLang
