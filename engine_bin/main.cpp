#include <box2d/box2d.h>
#include <raylib.h>

#include <game.h>

int main(void) {

	// Initialize game

	Game game;

	game_object = &game;

	game.name = "test";

	game.window_height = 600;
	game.window_width  = 800;

	// Initialize window
	InitWindow(game.window_width, game.window_height, game.name.c_str());
	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose()) {

		BeginDrawing();
		ClearBackground(RAYWHITE);

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
