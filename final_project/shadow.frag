#version 410
layout (location = 0) out vec4 shadowMap;

void main()
{
	shadowMap = vec4(gl_FragCoord.z);
}