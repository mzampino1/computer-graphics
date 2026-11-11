#version 410

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 uv;

smooth out vec2 uvCoords;

void main()
{
    uvCoords = uv;

    gl_Position = vec4(pos.xyz, 1.0);
}