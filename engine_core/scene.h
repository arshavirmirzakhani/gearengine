#pragma once
#include "ecs/object.h"
#include "globals.h"

class Scene {
	private:
	public:
		std::map<std::string, Object> objects;

		Scene() {};
		~Scene() {};
};
