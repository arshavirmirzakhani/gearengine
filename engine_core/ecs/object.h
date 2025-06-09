#pragma once

#include "objectType.h"
class Object {
	private:
	public:
		int x = 0, y = 0;

		ObjectType* type;

		std::map<std::string, Object> child_objects;

		Object() {};
		~Object() {};

		void process() {};
};
