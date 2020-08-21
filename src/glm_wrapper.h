// GLM was originally designed for OpenGL, so several defines are used to make it more Vulkan-compatible.
// These defines should be used for everything in this project.

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>