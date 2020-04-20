#pragma once

// debug flag for advanced debugging
#define DE_BUG 1
#define ID3D(x, y, z, h, w) (x + h * (y + w * z))
#define ID2D(x, y, w) (width * row + col)

// OpenGL API stuff
#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include <glfw3.h>

// GL math libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/integer.hpp>

// imgui stuff
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"
#include "vendor/imgui/imgui_internal.h"

// common std DS & A
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <concurrent_unordered_map.h>
#include <algorithm>
#include <chrono>
using namespace std::chrono;

// debugging
#include "engine_assert.h"
#include "debug.h"

// other common includes
#include <iostream>
#include <cstdlib>