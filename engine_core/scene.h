#pragma once
#include "globals.h"
#include "object.h"

class Scene {
	private:
	public:
		std::map<std::string, Object*> objects;

		Scene() {};
		~Scene() {};

		void init() {
			for (auto object : objects) {
				object.second->init();
			}
		};
		void process() {
			for (auto object : objects) {
				object.second->process();
			}
		};
};
