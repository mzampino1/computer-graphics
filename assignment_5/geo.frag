#version 410
smooth in mat3 TBN;
smooth in vec2 uvCoords;
smooth in vec3 global_pos;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 gbuff_normal;
layout (location = 2) out vec4 position;

uniform sampler2D albedo;
uniform sampler2D normal;

void main()
{
	color = texture(albedo, uvCoords);
	gbuff_normal = vec4(normalize(TBN * (texture(normal, uvCoords).xyz * 2.0f - 1.0f)), 1.0);
	position = vec4(global_pos, 1.0);
}