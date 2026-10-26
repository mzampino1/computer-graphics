#version 410

uniform sampler2D color;
uniform sampler2D gbuff_normal;
uniform sampler2D position;

smooth in vec2 uvCoords;

const int LIGHTS_COUNT = 10;
uniform vec3 lightPosition[LIGHTS_COUNT];
uniform vec3 lightColor[LIGHTS_COUNT];

out vec4 fragColor;

void main()
{
	// Fetch values saved by geo.frag
	vec3 fragPos  = texture(position, uvCoords).xyz;
	vec3 norm     = normalize(texture(gbuff_normal, uvCoords).xyz);
	vec3 albedo   = texture(color, uvCoords).xyz;

	// Do not light pixel if there is no geometry
	if (texture(position, uvCoords).w == 0.0) {
		fragColor = vec4(albedo, 1.0);
		return;
	}

	// Constants for attenuation
	const float c = 1.0;
	const float l = 0.09;
	const float q = 0.03;

	vec3 finalLighting = vec3(0.0);
	
	for (int i = 0; i < LIGHTS_COUNT; i++) {
		vec3 lightVec = lightPosition[i] - fragPos;
		// Compute distance from surface to light
		float dist = length(lightVec);
		float distSquared = dist * dist;

		float attenuation = 1.0 / (c + l*dist + q*distSquared);

		// Lambertian lighting
		float lambda = dot(normalize(norm), normalize(lightVec));
		lambda = clamp(lambda, 0.0, 1.0);

		finalLighting += attenuation * lambda * lightColor[i] * albedo;
	}

	fragColor = vec4(finalLighting, 1.0);
}