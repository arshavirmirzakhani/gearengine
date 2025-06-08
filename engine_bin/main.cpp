#include <box2d/box2d.h>
#include <raylib.h>

#include <game.h>

// Constants
const int screenWidth  = 800;
const int screenHeight = 450;
const float PPM	       = 100.0f; // Pixels per meter

int main(void) {
	// Initialize window
	InitWindow(screenWidth, screenHeight, "test");
	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose()) {

		BeginDrawing();
		ClearBackground(RAYWHITE);

		DrawText("hello world", 10, 10, 20, BLACK);

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
