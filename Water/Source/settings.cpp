#include "stdafx.h"
#include "input.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace ImGui;
//extern GLFWwindow* window;

// currently active options menu
enum OptionsTab : int
{
	kControls,
	kGraphics,
	kSound,
	//kEditor - omitted, will be modified in the editor itself
	kCount
};

static int activeMenu = kGraphics;