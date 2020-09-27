#include <cmath>
#include <glm/geometric.hpp>

#include "camera.h"

namespace cam {

	camera::camera(glm::vec3 inPos) : pos(inPos), hangle(90.0f), vangle(0.0f), skip(0), prevX(0.0), prevY(0.0), mscale(1), lscale(1) {
		prevTime = glfwGetTime();
	}

	camera::camera() : pos(glm::vec3(0.0f, 0.0f, -3.0f)), hangle(90.0f), vangle(0.0f), skip(0), prevX(0.0), prevY(0.0), mscale(1), lscale(1) {
		prevTime = glfwGetTime();
	}

	void camera::update(GLFWwindow *window) {

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		if (skip <= 1) { // prevent jumping to weird screen coords when mouse first comes into view
			prevX = x;
			prevY = y;
			skip++;
		}

		mscale = std::min(glfwGetTime() - prevTime, 0.05);
		lscale = std::min(glfwGetTime() - prevTime, 0.05);

		// target is 1 away from pos, always.  This makes math easier (hypotenuse = 1)

		float xoff = x - prevX;
		float yoff = prevY - y;

		prevX = x;
		prevY = y;

		float swapCoords = (vulkanCoords) ? -1.0f : 1.0f;

		if (mouseLook) {
			hangle += (float)(xoff * lscale) * swapCoords;
			vangle += (float)(yoff * lscale) * swapCoords;
		}

		// if using a laptop, sometimes the mouse can be annoying. keyboard versions of the same thing are set here.
		// (mouse still works while using the keyboard!)
		if (keyboardLook) {
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
				hangle -= 20.0f * lscale * swapCoords;
			}
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
				hangle += 20.0f * lscale * swapCoords;
			}
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				vangle += 20.0f * lscale * swapCoords;
			}
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				vangle -= 20.0f * lscale * swapCoords;
			}
		}
		
		// sine and cosine functions affect more than one axis

		glm::vec3 dir;
		dir.x = cosf(rads(vangle)) * cosf(rads(hangle));
		dir.y = sinf(rads(vangle));
		dir.z = cosf(rads(vangle)) * sinf(rads(hangle));

		front = glm::normalize(dir);
		
		glm::vec3 updir = glm::vec3(0.0f, 1.0f * swapCoords, 0.0f);
		glm::vec3 rdir = glm::cross(updir, front);

		// move camera according to user input
		// callback is annoying since it only triggers when key is updated, this is kinda cleaner and I don't really care if I miss a single keypress or two
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			pos += glm::vec3((float)mscale) * front; // can't multiply a scalar with a vector here...
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			pos -= glm::vec3((float)mscale) * front;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			pos += glm::vec3((float)mscale) * rdir;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			pos -= glm::vec3((float)mscale) * rdir;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			pos += glm::vec3((float)mscale) * updir;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			pos -= glm::vec3((float)mscale) * updir;
		}
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { // reset camera to original location
			pos = glm::vec3(0.0f);
			hangle = 90.0f;
			vangle = 0.0f;
		}
	}
};
