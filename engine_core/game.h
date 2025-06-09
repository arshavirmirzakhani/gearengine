#pragma once
#include "globals.h"
#include "scene.h"

class Game {
	private:
	public:
		// game name , also used for default window title
		std::string name;

		int window_width  = 0;
		int window_height = 0;

		// <scene name - scene object>
		std::map<std::string, Scene> scenes;
		std::string current_scene;

		Game() {};
		~Game() {};
};
