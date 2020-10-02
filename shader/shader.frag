#version 450

layout (location = 0) in vec4 color;
layout (location = 1) in vec2 tc;

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) out vec4 fColor;

void main() {
	fColor = mix(color, texture(tex, tc), 0.5f);
}
