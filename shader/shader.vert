#version 450

// for vulkan:
// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// depth goes (0, 1)
// gl_VertexIndex instead of gl_VertexID

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 vColor;

void main() {
	gl_Position = vec4(position, 1.0f);
	vColor = color;
}
