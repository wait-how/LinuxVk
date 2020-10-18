#version 460
#extension GL_EXT_shader_explicit_arithmetic_types: enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 n;
layout (location = 2) in vec2 uv;

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) out vec4 fColor;

struct light {
	vec3 pos;
	vec3 color;
};

const light l = light(vec3(0.0f, 0.0f, -2.0f), vec3(1.0f));

void main() {
	
	vec3 ldir = l.pos - pos;
	
	float dist = length(ldir);
	float falloff = 1.0 / dist;
	ldir /= dist;

	float diff = clamp(dot(ldir, normalize(n)), 0.0f, 1.0f);

	vec3 base = vec3(texture(tex, uv));
	vec3 amb = vec3(0.15f) * base;
	vec3 final = mix(amb, base, diff);

	// spec requires eye pos, and I don't want to
	// encode it in the origin by making everything in eye space

	fColor = min(vec4(final * falloff, 1.0f), vec4(1.0f));
}
