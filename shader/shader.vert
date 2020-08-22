#version 450

// for vulkan:
// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// depth goes (0, 1)
// gl_VertexIndex instead of gl_VertexID

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

layout (binding = 0) uniform uniformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) out vec4 vColor;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0f);
	vColor = vec4(normal, 1.0f);
}
