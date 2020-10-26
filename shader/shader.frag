#version 460

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec3 eye;
layout (location = 3) in vec2 uv;

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) out vec4 fragcolor;

struct point {
	vec3 p;
	vec3 color;
};

vec3 phong(in point l, in vec3 base) {
	vec3 ldir = l.p - p;
	
	float dist = length(ldir);

	const vec3 cf = vec3(0.0, 1.0, 0.0);
	float falloff = cf.x + (cf.y / dist) + (cf.z / (dist * dist));
	ldir /= dist;

	vec3 nn = normalize(n);

	float diff = clamp(dot(ldir, nn), 0.0, 1.0);

	vec3 amb = 0.15 * base;
	vec3 diffc = mix(amb, base * l.color, diff);

	// TODO: specular lighting

	return diffc * falloff;
}

void main() {

	const point l = point(vec3(0.0, 2.0, -2.0), vec3(1.0));

	const vec3 base = texture(tex, uv).rgb;

	vec3 lc = phong(l, base);

	fragcolor = vec4(min(lc, vec3(1.0)), 1.0);
}
