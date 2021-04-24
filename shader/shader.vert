#version 460
// use GL_EXT_shader_explicit_arithmetic_types for float16

// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// right handed system, -Y is up normally but I flipped the rasterizer
// depth goes from 0 to 1 as object gets farther away

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
// layout (location = 3) in vec3 tangent;

layout (set = 0, binding = 0) uniform uniformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 c;
} ubo;

layout (location = 0) out vec3 p;
layout (location = 1) out vec3 n;
layout (location = 2) out vec2 uv;
layout (location = 3) out vec3 eye;

void main() {
	vec4 p4 = ubo.model * vec4(position, 1.0);

	gl_Position = ubo.proj * ubo.view * p4;
	
	p = p4.xyz;
	n = mat3(ubo.model) * normal;
	uv = texcoord;
	eye = ubo.c;
}
