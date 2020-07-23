#version 450

// for vulkan:
// NDCs are (-1, -1) in top left to (1, 1) in bottom right
// depth goes (0, 1)

vec4 verts[] = {
	vec4(0.0, -1.0, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(-1.0, 1.0, 0.0, 1.0)
};

vec4 colors[] = {
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0)
};

layout (location = 0) out vec4 vcolor;

void main() {
	gl_Position = verts[gl_VertexIndex];
	vcolor = colors[gl_VertexIndex];
}
