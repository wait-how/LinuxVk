#version 460 core

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 eye;

layout (location = 4) in mat3 tbn;

// maps are defined as [diffuse, normal, displacement]
layout (set = 0, binding = 1) uniform sampler2D maps[3];

layout (location = 0) out vec4 fragcolor;

struct point {
	vec3 p;
	vec3 color;
};

vec2 disp_map(in vec2 uv) {
	const float scale = 0.1;
	vec3 tldir = normalize(transpose(tbn) * (eye - p)) * scale; // transpose == inverse for orthogonal matrix

	/*
	// number of iterations to try and find the surface of the displacement
	// (this is more efficient but looks worse from extreme angles)
	const uint minSamples = 2;
	const uint maxSamples = 16;
	float samples = mix(maxSamples, minSamples, max(dot(tldir, vec3(0.0, 0.0, 1.0)), 0.0));
	*/

	const uint samples = 16;

	const float pstep = 1.0 / samples;
	uint idx = 0;

	float d = texture(maps[2], uv).r; // assuming 1.0 == max height in disp map
	float td = 0;
	vec2 duv = uv;

	// iterate until we go "outside" the displacement map height
	while (d >= td && idx < samples) {
		idx++;
		duv += tldir.xy * pstep;
		d = texture(maps[2], duv).r;
		td += pstep;
	}

	// weight "before" and "after" uv offsets by how far away they are from their respective layers
	vec2 preuv = duv - tldir.xy * pstep;
	float pred = texture(maps[2], preuv).r - td + pstep;
	float currd = d - td;
	float w = currd / (currd - pred);
	
	return mix(duv, preuv, w);
}

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
	spec = pow(spec, 128);

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

	const point l = point(vec3(0.0, 2.0, 0.0), vec3(1.0));

	// displacement map
	vec2 duv = disp_map(uv);

	// normal map
	vec3 nt = texture(maps[1], duv).rgb;
	nt = normalize(nt * 2.0 - 1.0); // scale from [0, 1] -> [-1, 1]
	nt = tbn * nt; // map to world space

	// diffuse map
	vec3 c = texture(maps[0], duv).rgb;

	c = blinn_phong(l, c, nt);

	fragcolor = vec4(clamp(c, vec3(0.0), vec3(1.0)), 1.0);
}
