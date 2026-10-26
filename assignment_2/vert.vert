#version 410
layout (location=0) in vec3 pos;
layout (location=1) in vec3 color;
smooth out vec3 col;

void main()
{
	gl_Position = vec4(pos.x + 1, pos.y + 1, pos.z, 1.0f);
	col = color;
}