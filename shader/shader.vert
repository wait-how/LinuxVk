#version 460 core
// use GL_EXT_shader_explicit_arithmetic_types for float16

// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// right handed system, -Y is up normally but I flipped the rasterizer
// depth goes from 0 to 1 as object gets farther away

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec3 tangent;

layout (set = 0, binding = 0) uniform uniformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (push_constant) uniform push_data {
	vec3 c;
} pd;

layout (location = 0) out vec3 p;
layout (location = 1) out vec3 n;
layout (location = 2) out vec2 uv;
layout (location = 3) out vec3 eye;

layout (location = 4) out mat3 tbn;

void main() {
	vec4 p4 = ubo.model * vec4(position, 1.0);

	gl_Position = ubo.proj * ubo.view * p4;
	
	p = p4.xyz;
	n = mat3(ubo.model) * normal;
	uv = texcoord;
	eye = pd.c;

	// create a change of basis matrix to map normal map vertices to world space normals
	vec3 t = normalize(vec3(ubo.model * vec4(tangent, 0.0)));
	vec3 nfull = normalize(vec3(ubo.model * vec4(normal, 0.0)));
	vec3 b = cross(nfull, t);
	tbn = mat3(t, b, nfull);
}
