#version 410
layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

smooth out vec3 normal;
flat out vec3 position;

void main()
{
	gl_Position = projection * view * model * vec4(pos.xyz, 1.0);
	normal = norm;
	position = (model * vec4(pos.xyz, 1.0)).xyz;
}