#version 410

uniform sampler2D color;
uniform sampler2D gbuff_normal;
uniform sampler2D position;
uniform sampler2D shadowMap;

layout (std140) uniform LightMatrices
{
    mat4 lightView;
    mat4 lightOrtho;
};

uniform vec3 lightDir;

smooth in vec2 uvCoords;

out vec4 fragColor;

void main()
{
	// Fetch values saved by geo.frag
	vec3 worldPos  = texture(position, uvCoords).xyz;
	vec3 norm     = normalize(texture(gbuff_normal, uvCoords).xyz);
	vec3 albedo   = texture(color, uvCoords).xyz;

	// Do not light pixel if there is no geometry
	if (texture(position, uvCoords).w == 0.0) {
		fragColor = vec4(albedo, 1.0);
		return;
	}

	// Lambertian lighting
	float lambda = dot(normalize(norm), normalize(-lightDir));
	lambda = clamp(lambda, 0.0, 1.0);

	vec3 diffuse = lambda * albedo;

	// Apply shadows
	vec4 shadowSpace = lightOrtho * lightView * vec4(worldPos, 1.0);

	vec3 shadowUV_Depth = shadowSpace.xyz * 0.5 + 0.5;
	float shadowMapDepth = texture(shadowMap, shadowUV_Depth.xy).r;
	float thisDepth = shadowUV_Depth.z;

	const float bias = 0.01; // increase as needed
	float shadow = (thisDepth - bias > shadowMapDepth) ? 0.0 : 1.0;
	shadow = (thisDepth > 1.0) ? 1.0 : shadow;

	// Prevent shadow map wrapping (outside the light's box counts as lit)
	if (shadowUV_Depth.x < 0.0 || shadowUV_Depth.x > 1.0 ||
	    shadowUV_Depth.y < 0.0 || shadowUV_Depth.y > 1.0) {
		shadow = 1.0;
	}

	// Ambient keeps shadowed areas from being black
    vec3 ambient = 0.10 * albedo;

    fragColor = vec4(ambient + shadow * diffuse, 1.0);
}