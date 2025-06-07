#include <raylib.h>
#include <rcamera.h>

int main(void) {

	const int screenWidth  = 800;
	const int screenHeight = 450;

	InitWindow(screenWidth, screenHeight, "test");

	SetTargetFPS(60);

	while (!WindowShouldClose()) {

		BeginDrawing();

		DrawText("hello world", 100, 100, 20, BLACK);

		EndDrawing();
	}

	CloseWindow();

	return 0;
}