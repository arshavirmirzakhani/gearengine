#include <box2d/box2d.h>
#include <raylib.h>
#include <zip_file.hpp>

#include <game.h>

int main(void) {

	// Load game data
	miniz_cpp::zip_file data("data.gear");

	// Initialize game
	Game game;
	game_object = &game;

	game.name	   = "test";
	game.window_height = 600;
	game.window_width  = 800;

	// Initialize window
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(game.window_width, game.window_height, game.name.c_str());
	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose()) {

		game.process();

		BeginDrawing();
		ClearBackground(RAYWHITE);

		if (IsKeyPressed(KEY_D)) {
			game.debug = !game.debug;
		}

		if (game.debug) {
			DrawText(("FPS: " + std::to_string(GetFPS())).c_str(), 1, 1, 15, PINK);
		}

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
