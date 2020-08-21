#pragma once

#include "glm_wrapper.h"
#include "glfw_wrapper.h"

namespace cam {
	class test {
	public:
		test(glm::vec3 nv) : v(nv) {}
		float print() { return v.x; }
	private:
		glm::vec3 v;
	};

	class camera {
	public:
		glm::vec3 pos;
		glm::vec3 front;

		float hangle; // angles to keep track of camera direction
		float vangle;
		
		//camera(glm::vec3 inPos) : pos(inPos), hangle(90.0f), vangle(0.0f), skip(0), prevX(0.0), prevY(0.0) { }
		camera(glm::vec3 inPos);
		void update(GLFWwindow*);
	private:
		int skip;
		double prevX, prevY;
	};
};
