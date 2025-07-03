#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"
#include "PBRMetallicRoughness.glsl"


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

struct Empty{ float e; };

const float shadowFactor = 1.0f;

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
	
	// Advanced PCF with Poisson disk sampling
	vec2 poissonDisk[16] = vec2[](
		vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
		vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
		vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
		vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
		vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
		vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
		vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
		vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
	);
	
	float shadowFilterRadius = 2.0;
	for(int i = 0; i < 16; i++){
		vec2 offset = poissonDisk[i] * shadowFilterRadius * texelSize;
		float pcfDepth = texture(depthShadowMap, shadowTexCoord + offset).r;
		shadow += (currentDepth + bias < pcfDepth) ? 1.0f : 0.0f;
	}
	shadow /= 16.0;

	if(projCoords.z > 1.0)
        shadow = 1.0;

	return shadow;
}

vec3 pbr() {
	vec3 tex = pow(texture(colorTex,inUV).xyz, vec3(2.2f));
	vec3 albedo = tex * inColor;

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

	// Start with vertex normal
	vec3 N = normalize(inNormal);

	// Only apply normal mapping if we have a proper normal map texture
	vec4 normalFromTex = texture(normalTex, inUV);
	// Check if this is the default grey texture (0.66, 0.66, 0.66)
	if (length(normalFromTex.rgb - vec3(0.66)) > 0.1) {
		vec3 normFromTex = normalFromTex.xyz;
		normFromTex = normFromTex * 2.0f - 1.0f;

		// Normalized tangent and bitangent
		vec3 tangent = normalize(inTangent);
		vec3 bitangent = normalize(inBitangent);

		// Construct TBN matrix
		mat3 TBN = mat3(tangent, bitangent, N);

		// Apply normal mapping
		N = normalize(TBN * normFromTex);
	}

	vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPos);

	vec3 L = - normalize(sceneData.sunlightDirection.xyz); // lightDirection is a uniform pointing to the light

	vec3 bsdf = BSDF(metallic, roughness, N, V, L, albedo);
	bsdf *= sceneData.sunlightDirection.w * sceneData.sunlightColor.rgb;

	// ambient lighting
	vec2 screenUV = gl_FragCoord.xy / textureSize(ssaoMap, 0);
	vec3 ssao = texture(ssaoMap, screenUV).xxx;
	
	// Proper ambient term for PBR: non-metals use albedo, metals use minimal ambient
	vec3 kS = mix(vec3(0.04), albedo, metallic); // Base reflectivity
	vec3 kD = (1.0 - kS) * (1.0 - metallic); // Diffuse contribution
	vec3 ambient = kD * albedo * sceneData.ambientColor.xyz;

	if (bool(sceneData.enableSSAO == 0)) {
		ssao = vec3(1.0f);
	}
	ambient *= ssao;

	// Shadow calculation
	float shadow = shadowCalculation(inFragPosLightSpace, N, L);

	if (bool(sceneData.enableShadows == 0)) {
		shadow = 0.0f;
	}

	vec3 color = ambient + bsdf * (1.0 - shadow * shadowFactor);

	return color;
}

vec3 blinnPhong() {

	vec3 tex = pow(texture(colorTex,inUV).xyz, vec3(2.2f));
	vec3 color = tex * inColor;

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
	vec2 screenUV = gl_FragCoord.xy / textureSize(ssaoMap, 0);
	vec3 ssao = texture(ssaoMap, screenUV).xxx;
	vec3 ambient = color *  sceneData.ambientColor.xyz;

	// Start with vertex normal
	vec3 normalMap = normalize(inNormal);

	// Only apply normal mapping if we have a proper normal map texture
	vec4 normalFromTex = texture(normalTex, inUV);
	// Check if this is the default grey texture (0.66, 0.66, 0.66)
	if (length(normalFromTex.rgb - vec3(0.66)) > 0.1) {
		vec3 normFromTex = normalFromTex.xyz;
		normFromTex = normFromTex * 2.0f - 1.0f;

		// Normalized tangent and bitangent
		vec3 tangent = normalize(inTangent);
		vec3 bitangent = normalize(inBitangent);

		// Construct TBN matrix
		mat3 TBN = mat3(tangent, bitangent, normalMap);

		// Apply normal mapping
		normalMap = normalize(TBN * normFromTex);
	}

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
	float shadow = shadowCalculation(inFragPosLightSpace, normalMap, sunlightDir);

	// Specular light calc for blinn-phong specular
	spec = blinn_specular(max(dot(normalMap, halfwayDir), 0.0), specular, roughness);

	if(bool(sceneData.enableShadows == 0)) {
		shadow = 0.0f;
	}
	if (bool(sceneData.enableSSAO == 0)) {
		ssao = vec3(1.0f);
	}

	// Final color
	vec3 lighting = ((ambient*ssao) + (1.0 - (shadow * shadowFactor)) * (diffuse + spec));

	return lighting;
}

void crashMethod2() {
    vec3 color = texture(colorTex, inUV).rgb;
    
    // Create exponentially expensive computation
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 1000; j++) {
            color = normalize(color + sin(vec3(i * j + gl_FragCoord.x)));
            color *= 1.0001; // Prevent optimization
        }
    }
    outFragColor = vec4(color, 1.0);
}


void main() {

	//crashMethod2(); // Uncomment to test infinite loop crash

	vec3 color = vec3(0.0f, 0.0f, 0.0f);
	float alpha = texture(colorTex, inUV).a;
    
    // Discard fragments that are nearly transparent
    if (alpha < 0.01f) {
        discard;
    }
	if(bool(sceneData.enablePBR)) {
		color = pbr();
	}
	else {
		color = blinnPhong();
	}

	outFragColor = vec4(color, 1.0f);
}
