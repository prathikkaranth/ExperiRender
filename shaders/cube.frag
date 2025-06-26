#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    mat4 lightSpaceMatrix;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec3 cameraPosition;
    int enableShadows;
    int enableSSAO;
    int enablePBR;
} sceneData;

void main() {
    vec3 normal = normalize(fragNormal);
    
    // Simple lighting calculation
    vec3 lightDir = normalize(-sceneData.sunlightDirection.xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);
    
    // Ambient + diffuse lighting
    vec3 ambient = sceneData.ambientColor.rgb * fragColor;
    vec3 diffuse = sceneData.sunlightColor.rgb * ndotl * fragColor;
    
    vec3 finalColor = ambient + diffuse;
    
    outColor = vec4(finalColor, 1.0);
}