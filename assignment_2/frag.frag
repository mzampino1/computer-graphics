#version 410
smooth in vec3 col;
out vec4 color;

void main()
{
	color = vec4(col.xyz, 1.0f);
}