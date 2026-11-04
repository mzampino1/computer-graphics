#version 410
layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform sampler2D height;

layout (std140) uniform LightMatrices
{
    mat4 lightView;
    mat4 lightOrtho;
};

void main()
{
	const float amplitude = 0.4; // must match terrain.vert
	vec2 hUV = clamp(uv, 0.5 / 1024.0, 1.0 - 0.5 / 1024.0);
	vec3 offset = amplitude * texture(height, hUV).x * vec3(0.0, 1.0, 0.0);
	vec3 position = pos + offset;
	gl_Position = lightOrtho * lightView * model * vec4(position, 1.0);
}