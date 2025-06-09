#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"

int main(int argc, char* argv[]) {
	int screenWidth	 = 1280;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "Gear-engine editor");
	SetTargetFPS(144);
	rlImGuiSetup(true);

	auto io		= &ImGui::GetIO();
	io->IniFilename = NULL;
	io->LogFilename = NULL;

	// Main game loop
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(DARKGRAY);

		rlImGuiBegin();

		bool open = true;
		ImGui::ShowDemoWindow(&open);

		rlImGuiEnd();

		EndDrawing();
	}

	rlImGuiShutdown();
	CloseWindow();

	return 0;
}