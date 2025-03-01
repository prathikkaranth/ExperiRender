#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"


layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalTex;
layout(set = 1, binding = 4) uniform sampler2D ssaoMap;
layout(set = 1, binding = 5) uniform sampler2D depthShadowMap;

layout(set = 2, binding = 0) uniform sampler2D gbufferPosMap;
layout(set = 2, binding = 1) uniform sampler2D gbufferNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec3 inBitangent;
layout (location = 6) in vec4 inFragPosLightSpace;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragWorldPos;
layout (location = 2) out vec4 outFragWorldNormal;

struct Empty{ float e; };

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	Empty e[];
} PushConstants;

// blinn-phong specular
vec3 blinn_specular(in float Ndh, in vec3 specular, in float roughness) {
	float k = 1.999f/ (roughness * roughness);

	return min(1.0, 3.0 * 0.0398 * k) * pow(Ndh, min(10000.0, k)) * specular;
}

// shadow calculation
float shadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir){
	
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	vec2 shadowTexCoord = projCoords.xy * 0.5f + 0.5f;

	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(depthShadowMap, shadowTexCoord).r;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// float shadow = (currentDepth < closestDepth) ? 1.0f : 0.0f;
	
	// check whether current frag pos is in shadow
	const float minBias = 0.0001f;
    const float maxBias = 0.001f;

	// float bias = max(maxBias * (1.0 - dot(normal, lightDir)), minBias);  
	// float shadow = currentDepth + bias < closestDepth  ? 1.0 : 0.0; 
    const float bias = max(maxBias * (1.0f - dot(normal, lightDir)), minBias);
	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(depthShadowMap, 0);
	for(int x = -1; x <= 1; ++x){
		for(int y = -1; y <= 1; ++y){
			float pcfDepth = texture(depthShadowMap, shadowTexCoord + vec2(x, y) * texelSize).r;
			shadow += (currentDepth + bias < pcfDepth) ? 1.0f : 0.0f;
		}
	} 
	shadow /= 9.0f;

	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}


void main() 
{
	vec3 color = inColor * texture(colorTex,inUV).xyz;

	// Metallic
	float metallic = 0;
	if(bool(materialData.hasMetalRoughTex))
		metallic = texture(metalRoughTex, inUV).x * materialData.metal_rough_factors.x;
	else
		metallic = materialData.metal_rough_factors.x;

	// Roughness
	float roughness = 0;
	if(bool(materialData.hasMetalRoughTex))
		roughness = texture(metalRoughTex, inUV).y * materialData.metal_rough_factors.y;
	else
		roughness = materialData.metal_rough_factors.y;
	
	// mix between metal and non-metal material, for non-metal
    // constant base specular factor of 0.04 grey is used
	vec3 specular = mix(vec3(0.04), color, metallic);


	// Ambient light
	vec2 screenUV = gl_FragCoord.xy / vec2(1600, 900);
	vec3 ssao = texture(ssaoMap, screenUV).xxx;
	vec3 ambient = color *  sceneData.ambientColor.xyz;

	// Normalized normal
	vec4 normalFromTex = texture(normalTex, inUV);
	vec3 normFromTex = normalFromTex.xyz;
	normFromTex = normFromTex * 2.0f - 1.0f;
	vec3 normal = normalize(inNormal);

	// Normalized tangent and bitangent
	vec3 tangent = normalize(inTangent);
	vec3 bitangent = normalize(inBitangent);

	// Construct TBN matrix
	mat3 TBN = mat3(tangent, bitangent, normal);

	// Normal map
	vec3 normalMap = normalize(TBN * normFromTex);

	// Diffuse light
	vec3 sunlightDir = - normalize(sceneData.sunlightDirection.xyz);
	float diff = max(dot(sunlightDir, normalMap), 0.0f);
	vec3 diffuse = diff * color * sceneData.sunlightColor.xyz * sceneData.sunlightDirection.w;

	// View direction
	vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);

	// blinn-phong specular
	vec3 halfwayDir = normalize(sunlightDir + viewDir);
	vec3 spec = vec3(0.0f, 0.0f, 0.0f);

	// Shadow calculation
	float shadow = shadowCalculation(inFragPosLightSpace, normal, sunlightDir);
	float shadowFactor = 0.85f;

	// Specular light calc for blinn-phong specular
	spec = blinn_specular(max(dot(normalMap, halfwayDir), 0.0), specular, roughness);

	if(bool(sceneData.enableShadows == 0)) {
		shadow = 0.0f;
	}
	if (bool(sceneData.enableSSAO == 0)) {
		ssao = vec3(1.0f);
	}

	// Final color
	vec3 lighting = ((ambient*ssao) + (1.0 - shadow * shadowFactor) * (diffuse + spec));
	outFragColor = vec4(lighting, 1.0f);

}