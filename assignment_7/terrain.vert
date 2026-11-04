#version 410

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

uniform sampler2D height;
out vec2 texCoord;
out vec3 vWorldPos;

void main()
{
	texCoord = uv;
	const float amplitude = 0.4;
	vec2 hUV = clamp(uv, 0.5 / 1024.0, 1.0 - 0.5 / 1024.0);
	vec3 offset = amplitude * texture(height, hUV).x * vec3(0.0, 1.0, 0.0);
	vec3 position = pos.xyz + offset;
	gl_Position = projection * view * model * vec4(position.xyz, 1.0);
	vWorldPos = (model * vec4(position, 1.0)).xyz;
}