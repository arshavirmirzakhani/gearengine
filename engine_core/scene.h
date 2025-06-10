#pragma once
#include "globals.h"
#include "object.h"

class Scene {
	private:
	public:
		std::map<std::string, Object*> objects;

		Scene() {};
		~Scene() {};

		void init() {};
		void process() {};
};
