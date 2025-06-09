#pragma once
#include "globals.h"
#include "scene.h"

class Game {
	private:
	public:
		// debug mode
		bool debug = false;

		// game name , also used for default window title
		std::string name;

		int window_width  = 0;
		int window_height = 0;

		// game assets <assets name - asset path>
		std::map<std::string, std::string> assets;

		// <scene name - scene object>
		std::map<std::string, Scene> scenes;
		std::string current_scene;

		Game() {};
		~Game() {};

		void process();
};

void Game::process() {
	for (auto object : scenes[current_scene].objects) {
		object.second.process();
	}
}