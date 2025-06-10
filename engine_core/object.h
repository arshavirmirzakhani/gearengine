#pragma once
#include "globals.h"

class Component {
	public:
		Component() {}
		~Component() {}
		virtual void init() {}
		virtual void process() {}
};

class spriteComponent : public Component {
	public:
		spriteComponent() {}
		~spriteComponent() {}
		virtual void init() {}
		virtual void process() {}
};

class Object {
	public:
		int x = 0, y = 0;
		std::vector<Component*> components;
		std::vector<Object*> children;
		Object* parent;

		sol::state lua_state;

		std::string script;

		Object() {
			lua_state.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os);

			lua_state.set_function("is_key_pressed", IsKeyPressed);
			lua_state.set_function("is_key_released", IsKeyReleased);
			lua_state.set_function("is_key_down", IsKeyDown);
			lua_state.set_function("is_key_up", IsKeyUp);
		};

		void init() { lua_state.script(script); }

		void process() {
			lua_state["DELTA"] = GetFrameTime();

			lua_state["POSITION_X"] = &x;
			lua_state["POSITION_Y"] = &y;

			lua_state.script("main()");
		};
};
