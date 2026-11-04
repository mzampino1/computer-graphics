#version 410

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 texCoord[];
in vec3 vWorldPos[];

out vec3 normal;
out vec3 worldPos;
out vec2 uvs;

void main()
{
	vec3 AB = vWorldPos[1] - vWorldPos[0];
	vec3 AC = vWorldPos[2] - vWorldPos[0];
	vec3 n = normalize(cross(AB, AC)); // Same for all vertices

	gl_Position = gl_in[0].gl_Position;
	uvs = texCoord[0];
	worldPos = vWorldPos[0];
	normal = n;
	EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	uvs = texCoord[1];
	worldPos = vWorldPos[1];
	normal = n;
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	uvs = texCoord[2];
	worldPos = vWorldPos[2];
	normal = n;
	EmitVertex();

	EndPrimitive();
}