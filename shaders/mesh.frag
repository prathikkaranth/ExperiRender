#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragWorldPos;
layout (location = 2) out vec4 outFragWorldNormal;

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
	float metallic = materialData.metal_rough_factors.x;

	float roughness = 0;
	// Roughness
	if(materialData.hasMetalRoughTex)
		roughness = texture(metalRoughTex, inUV).y * materialData.metal_rough_factors.y;
	else
		roughness = materialData.metal_rough_factors.y;
	
	// mix between metal and non-metal material, for non-metal
    // constant base specular factor of 0.04 grey is used
	vec3 specular = mix(vec3(0.04), color, metallic);


	// Ambient light
	vec3 ambient = color *  sceneData.ambientColor.xyz;

	// Normalized normal
	vec3 normal = normalize(inNormal);

	// Diffuse light
	float diff = max(dot(sceneData.sunlightDirection.xyz, normal), 0.0f);
	vec3 diffuse = diff * color * sceneData.sunlightColor.xyz;

	// Specular light
	vec3 specularStrength = vec3(0.3f, 0.3f, 0.3f);
	vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);
	vec3 reflectDir = reflect(-sceneData.sunlightDirection.xyz, normal);

	// phong specular
	// float specular = pow(max(dot(viewDir, reflectDir), 0.0), 4);

	// blinn-phong specular
	vec3 halfwayDir = normalize(sceneData.sunlightDirection.xyz + viewDir);
	// float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
	vec3 specFresnel = fresnelFactor(specular, max(dot(normal, viewDir), 0.0));
	vec3 spec = blinn_specular(max(dot(normal, halfwayDir), 0.0), specular, roughness);
	// vec3 specular = specularStrength * spec * sceneData.sunlightColor.xyz;


	outFragColor = vec4(spec + ambient + diffuse, 1.0f);
	outFragWorldPos = vec4(inWorldPos,1.0f);
	outFragWorldNormal = vec4(normal,1.0f);
}