#version 410
smooth in mat3 TBN;
smooth in vec2 uvCoords;
out vec4 color;
uniform vec3 lightDir;
uniform sampler2D albedo;
uniform sampler2D normal;

void main()
{
	vec3 norm = normalize(texture(normal, uvCoords).xyz * 2.0f - 1.0f);
	norm = normalize(TBN * norm);

	// Lambertian lighting
    float lambda = -dot(normalize(norm), normalize(lightDir));
	lambda = clamp(lambda, 0.0, 1.0);

	color = vec4(lambda * texture(albedo, uvCoords).xyz, 1.0f);
}