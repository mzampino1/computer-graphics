#version 410
layout (location = 0) in vec3 pos;

uniform mat4 model;

layout (std140) uniform LightMatrices
{
    mat4 lightView;
    mat4 lightOrtho;
};

void main()
{
	gl_Position = lightOrtho * lightView * model * vec4(pos, 1.0);
}