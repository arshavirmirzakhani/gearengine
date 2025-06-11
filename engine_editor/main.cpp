#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

int main(int argc, char* argv[]) {
	int screenWidth	 = 1280;
	int screenHeight = 800;

	SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "Gear-engine editor");

	SetTargetFPS(144);
	rlImGuiSetup(true);

	auto io		= &ImGui::GetIO();
	io->IniFilename = NULL;
	io->LogFilename = NULL;

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(DARKGRAY);

		rlImGuiBegin();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(GetScreenWidth(), GetScreenHeight()));

		ImGuiWindowFlags mainWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
						   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::Begin("Main Window", nullptr, mainWindowFlags);

		// Left Panel Child
		ImGui::BeginChild("Left Panel", ImVec2(250, 0), true);
		ImGui::Text("This is the left panel.");
		if (ImGui::Button("Button 1")) {
		}
		if (ImGui::Button("Button 2")) {
		}
		ImGui::EndChild();

		ImGui::SameLine(); // place next child to the right

		// Main Content Child
		ImGui::BeginChild("Main Content", ImVec2(0, 0), true); // width=0 takes remaining space
		ImGui::Text("This is the main content area.");
		ImGui::EndChild();

		ImGui::End();

		rlImGuiEnd();
		EndDrawing();
	}

	rlImGuiShutdown();
	CloseWindow();

	return 0;
}
