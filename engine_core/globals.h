#pragma once
#include <box2d/box2d.h>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <raylib.h>
#include <sol/sol.hpp>
#include <string>
#include <toml.hpp>
#include <vector>

class Game;

Game* game_object;
