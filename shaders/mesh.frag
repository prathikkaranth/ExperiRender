#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"


layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalTex;
layout(set = 1, binding = 4) uniform sampler2D ssaoMap;

layout(set = 2, binding = 0) uniform sampler2D gbufferPosMap;
layout(set = 2, binding = 1) uniform sampler2D gbufferNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec3 inBitangent;

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

vec3 fresnelFactor(in vec3 f0, in float product) {
	return mix(f0, vec3(1.0), pow(1.01 - product, 5.0));
}

vec3 blinn_specular(in float Ndh, in vec3 specular, in float roughness) {
	float k = 1.999f/ (roughness * roughness);

	return min(1.0, 3.0 * 0.0398 * k) * pow(Ndh, min(10000.0, k)) * specular;
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

	float roughness = 0;
	// Roughness
	if(bool(materialData.hasMetalRoughTex))
		roughness = texture(metalRoughTex, inUV).y * materialData.metal_rough_factors.y;
	else
		roughness = materialData.metal_rough_factors.y;
	
	// mix between metal and non-metal material, for non-metal
    // constant base specular factor of 0.04 grey is used
	vec3 specular = mix(vec3(0.04), color, metallic);


	// Ambient light
	vec2 screenUV = gl_FragCoord.xy / vec2(1280, 720);
	vec3 ssao = texture(ssaoMap, screenUV).xxx;
	vec3 ambient = color *  sceneData.ambientColor.xyz * ssao;

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
	float diff = max(dot(sceneData.sunlightDirection.xyz, normalMap), 0.0f);
	vec3 diffuse = diff * color * sceneData.sunlightColor.xyz * sceneData.sunlightDirection.w;

	// View direction
	vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);

	// blinn-phong specular
	vec3 halfwayDir = normalize(sceneData.sunlightDirection.xyz + viewDir);
	vec3 specFresnel = fresnelFactor(specular, max(dot(normalMap, viewDir), 0.0));
	vec3 spec = vec3(0.0f, 0.0f, 0.0f);

	if(bool(sceneData.hasSpecular)){
		spec = blinn_specular(max(dot(normalMap, halfwayDir), 0.0), specular, roughness);
		outFragColor = vec4(spec + ambient + diffuse, 1.0f);
	}
	else if(bool(sceneData.viewSSAOMAP)){
		// SSAO Map
		outFragColor = vec4(ssao, 1.0f);
	}
	else if(bool(sceneData.viewGbufferPos)){
		// G buffer World position
		vec3 gbufferPos = texture(gbufferPosMap, screenUV).xyz;
		outFragColor = vec4(gbufferPos, 1.0f);
	}
	else{
		// Final color
		outFragColor = vec4(ambient + diffuse, 1.0f);
	}
	

}