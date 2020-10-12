#version 450
// allow use of specific sizes for variables (float16_t, i8vec4, etc.)
#extension GL_EXT_shader_explicit_arithmetic_types: enable

// for vulkan:
// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// 		(right handed system, -Y is up)
// depth goes from 0 to 1 as object gets farther away
// gl_VertexIndex instead of gl_VertexID

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec3 tangent;

layout (set = 0, binding = 0) uniform uniformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) out vec3 pos;
layout (location = 1) out vec3 n;
layout (location = 2) out vec2 uv;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0f);
	
	pos = vec3(ubo.model * vec4(position, 1.0f));

	vec4 vn = mat4(mat3(ubo.model)) * vec4(normalize(normal), 1.0f);
	n = vec3(vn);
	
	uv = texcoord;
}
