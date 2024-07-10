#pragma once

#include <filesystem>

#include "game.h"

namespace json_loader {

application::game::Game LoadGame(const std::filesystem::path& json_path, bool is_random_spawn);

}  // namespace json_loader
