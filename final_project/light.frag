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

uniform samplerCube skybox;
uniform mat4 invViewProj;
uniform vec3 cameraPos;

smooth in vec2 uvCoords;

out vec4 fragColor;

void main()
{
	// Fetch values saved by geo.frag
	vec3 worldPos  = texture(position, uvCoords).xyz;
	vec3 norm     = normalize(texture(gbuff_normal, uvCoords).xyz);
	vec3 albedo   = texture(color, uvCoords).xyz;

	// Use skybox if there is no geometry
	if (texture(position, uvCoords).w == 0.0) {
		vec4 farPoint = invViewProj * vec4(uvCoords * 2.0 - 1.0, 1.0, 1.0);
		vec3 viewDir = normalize(farPoint.xyz / farPoint.w - cameraPos);
		fragColor = vec4(texture(skybox, viewDir).rgb, 1.0);
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

	const float bias = 0.001; // increase as needed
	float shadow = (thisDepth - bias > shadowMapDepth) ? 0.0 : 1.0;
	shadow = (thisDepth > 1.0) ? 1.0 : shadow;

	// Prevent shadow map wrapping (outside the light's box counts as lit)
	if (shadowUV_Depth.x < 0.0 || shadowUV_Depth.x > 1.0 ||
	    shadowUV_Depth.y < 0.0 || shadowUV_Depth.y > 1.0) {
		shadow = 1.0;
	}

	// Ambient keeps shadowed areas from being black
    vec3 ambient = 0.3 * albedo;

    vec3 litColor = ambient + shadow * diffuse;

	// Glassy marbles: Fresnel-weighted sky reflection with a sun highlight
	float reflectivity = texture(gbuff_normal, uvCoords).w;
	if (reflectivity > 0.0) {
		vec3 V = normalize(worldPos - cameraPos);
		vec3 R = reflect(V, norm);
		float fresnel = 0.3 + 0.7 * pow(1.0 - clamp(dot(-V, norm), 0.0, 1.0), 3.0);
		float spec = pow(max(dot(R, normalize(-lightDir)), 0.0), 48.0) * shadow * 1.5;
		litColor = mix(litColor, texture(skybox, R).rgb, fresnel * reflectivity) + spec * reflectivity;
	}

	fragColor = vec4(litColor, 1.0);
}