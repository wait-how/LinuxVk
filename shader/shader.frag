#version 460

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 eye;

layout (location = 4) in mat3 tbn;

// maps are defined as [diffuse, normal, ...]
layout (set = 0, binding = 1) uniform sampler2D maps[2];

layout (location = 0) out vec4 fragcolor;

struct point {
	vec3 p;
	vec3 color;
};

vec3 blinn_phong(in point l, in vec3 c, in vec3 nn) {
	vec3 ldir = l.p - p;
	
	float dist = length(ldir);

	const vec3 cf = vec3(0.0, 1.0, 0.0);
	float falloff = cf.x + (cf.y / dist) + (cf.z / (dist * dist));
	ldir /= dist;

	float diff = clamp(dot(ldir, nn), 0.0, 1.0);

	vec3 amb = 0.15 * c;
	vec3 diffc = mix(amb, c * l.color, diff);

	vec3 eyedir = normalize(eye - p);

	// blinn_phong calculates specular highlights from a vector halfway between
	// the light vector and the eye vector, meaning an angle between the
	// reflected light dir and the eye dir that is over 90° doesn't become zero.
	vec3 lhalf = normalize(ldir + eyedir);

	float spec = clamp(dot(lhalf, nn), 0.0, 1.0);
	spec = pow(spec, 150);

	vec3 specc = l.color * spec;

	return (diffc + specc) * falloff;
}

/*
vec3 fog(in float start, in float end, in vec3 c) {
	float depth = smoothstep(start, end, length(eye - p));
	return mix(c, vec3(0.15), depth);
}
*/

void main() {

	const point l = point(vec3(0.0, 2.0, -2.0), vec3(1.0));

	vec3 c = texture(maps[0], uv).rgb;
	vec3 nt = texture(maps[1], uv).rgb;
	nt = normalize(nt * 2.0 - 1.0); // scale from [0, 1] -> [-1, 1]
	nt = tbn * nt; // map to world space

	c = blinn_phong(l, c, nt);
	// c = fog(3.0, 7.0, c);

	fragcolor = vec4(min(c, vec3(1.0)), 1.0);
}
