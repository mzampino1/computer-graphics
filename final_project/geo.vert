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

smooth out mat3 TBN;
smooth out vec2 uvCoords;
smooth out vec3 global_pos;

void main()
{
	gl_Position = projection * view * model * vec4(pos.xyz, 1.0);
	vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
	vec3 B = normalize(vec3(model * vec4(bitangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(norm, 0.0)));
	T = normalize(T - N * dot(T, N));
	B = cross(N, T);
	TBN = mat3(T, B, N);

	global_pos = (model * vec4(pos, 1.0)).xyz;

	uvCoords = vec2(uv.x, uv.y);
}