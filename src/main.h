#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include <optional> // C++17, for device queue querying
#include <set>
#include <array> // for returning arrays of things

#include <cstring> // for strcmp
#include <cstdint> // for UINT32_MAX

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "vloader.h"

