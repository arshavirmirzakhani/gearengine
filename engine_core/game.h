#pragma once
#include "globals.h"
#include "scene.h"

class Game {
	private:
	public:
		// game name , used for window title
		std::string name;

		// <scene name - scene object>
		std::map<std::string, Scene> scenes;

		std::string current_scene_name;

		Game();
		~Game();
};
