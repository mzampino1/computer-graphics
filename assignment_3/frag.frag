#version 410
smooth in vec3 normal;
flat in vec3 position;
out vec4 color;
uniform vec3 lightDir;

void main()
{
	// Lambertian lighting
	vec3 diff = lightDir - position;
    float lambda = dot(normalize(normal), normalize(diff));
	lambda = clamp(lambda, 0.175, 1.0);

	color = vec4(lambda*vec3(0, 0.5, 0), 1.0);
}