#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    mat4 lightSpaceMatrix;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 pointLightPosition;
    vec4 pointLightColor;
    vec3 cameraPosition;
    int enableShadows;
    int enableSSAO;
    int enablePBR;
} sceneData;

void main() {
    vec3 normal = normalize(fragNormal);
    
    // Sunlight calculation
    vec3 sunLightDir = normalize(-sceneData.sunlightDirection.xyz);
    float sunNdotL = max(dot(normal, sunLightDir), 0.0);
    
    // Point light calculation
    vec3 pointLightDir = sceneData.pointLightPosition.xyz - fragWorldPos;
    float pointLightDistance = length(pointLightDir);
    pointLightDir = normalize(pointLightDir);
    float pointNdotL = max(dot(normal, pointLightDir), 0.0);
    
    // Point light attenuation
    float range = sceneData.pointLightPosition.w;
    float attenuation = 1.0 / (1.0 + pointLightDistance * pointLightDistance / (range * range));
    
    // Combine lighting
    vec3 ambient = sceneData.ambientColor.rgb * fragColor;
    vec3 sunDiffuse = sceneData.sunlightColor.rgb * sceneData.sunlightDirection.w * sunNdotL * fragColor;
    vec3 pointDiffuse = sceneData.pointLightColor.rgb * sceneData.pointLightColor.w * pointNdotL * attenuation * fragColor;
    
    vec3 finalColor = ambient + sunDiffuse + pointDiffuse;
    
    outColor = vec4(finalColor, 1.0);
}