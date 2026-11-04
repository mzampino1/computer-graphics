#version 410
in vec2 uvs;
in vec3 normal;
in vec3 worldPos;

uniform sampler2D height;
uniform sampler2D atlas;

// gBuffer outputs
layout (location = 0) out vec4 color;
layout (location = 1) out vec4 gbuff_normal;
layout (location = 2) out vec4 position;

const float LOW_ELEVATION = 0.25;
const float MID_ELEVATION = 0.5;
const float HIGH_ELEVATION = 0.75;

void main()
{
	float flatness = max(dot(normal, vec3(0, 1, 0)), 0.0);
	float displaced = texture(height, uvs).r;

	// Compute a uv offset based on the displaced heightmap sample and ELEVATIONs
	float row = step(LOW_ELEVATION, displaced)
	          + step(MID_ELEVATION, displaced)
	          + step(HIGH_ELEVATION, displaced);

	vec2 splat0 = 0.25 * uvs;
	splat0.y += row * 0.25; // uv Offset for elevation
	vec2 splat1 = splat0;
	splat1.x += 0.25; // Splat1 is the second texture – i.e., adjacent sub-texture in atlas

	vec4 diffuse = (flatness * texture(atlas, splat0) + 
				   (1.0 - flatness) * texture(atlas, splat1));
	
	color        = vec4(diffuse.xyz, 1.0);
	gbuff_normal = vec4(normal, 1.0);
	position     = vec4(worldPos, 1.0);
}