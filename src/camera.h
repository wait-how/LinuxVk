#pragma once

#include "glm_wrapper.h"
#include "glfw_wrapper.h"

namespace cam {
	constexpr bool keyboardLook = true;
	constexpr bool mouseLook = false;
	constexpr bool vulkanCoords = true;

	class camera {
	public:
		glm::vec3 pos;
		glm::vec3 front;

		float hangle; // angles to keep track of camera direction
		float vangle;
		
		//camera(glm::vec3 inPos) : pos(inPos), hangle(90.0f), vangle(0.0f), skip(0), prevX(0.0), prevY(0.0) { }
		camera(glm::vec3 inPos);
		camera();

		void update(GLFWwindow*);
	private:
		int skip;
		double prevX, prevY;
	};
};
