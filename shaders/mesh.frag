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


void main() 
{
	float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.0f);
	vec3 color = inColor * texture(colorTex,inUV).xyz;

	// Ambient light
	vec3 ambient = color *  sceneData.ambientColor.xyz;

	// Diffuse light
	vec3 diffuse = sceneData.sunlightColor.xyz * lightValue * color;

	// Specular light
	vec3 specularStrength = vec3(0.5f, 0.5f, 0.5f);
	vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);
	vec3 reflectDir = reflect(-sceneData.sunlightDirection.xyz, inNormal);

	// phong specular
	// float specular = pow(max(dot(viewDir, reflectDir), 0.0), 4);

	// blinn-phong specular
	vec3 halfwayDir = normalize(sceneData.sunlightDirection.xyz + viewDir);
	float spec = pow(max(dot(inNormal, halfwayDir), 0.0), 32.0);
	vec3 specular = specularStrength * spec;


	outFragColor = vec4(diffuse + ambient + specular, 1.0f);
	outFragWorldPos = vec4(inWorldPos,1.0f);
	outFragWorldNormal = vec4(inNormal,1.0f);
}