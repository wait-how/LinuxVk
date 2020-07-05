#pragma once

#include <GLFW/glfw3.h> // include this in order to get access to window dimensions
#include <glm/glm.hpp>
#include <cmath> // cmath is weird here and doesn't have M_PI, avoid it by defining pi ourselves

class camera {
public:
	glm::vec3 pos;
	glm::vec3 front;

	float hangle; // angles to keep track of camera direction
	float vangle;
	
	camera(glm::vec3 inPos);
	void update(GLFWwindow*);
private:
	int skip;
	double prevX, prevY;
};
