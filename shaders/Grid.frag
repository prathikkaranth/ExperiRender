#version 450

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

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
} sceneData;

vec3 worldPosFromScreenSpace(vec2 uv) {
    vec2 ndc = uv * 2.0 - 1.0;
    
    vec3 cameraPos = sceneData.cameraPosition;
    
    vec4 clipNear = vec4(ndc, -1.0, 1.0);  
    vec4 clipFar = vec4(ndc, 1.0, 1.0);   

    mat4 invViewProj = inverse(sceneData.viewproj);
    vec4 worldNear = invViewProj * clipNear;
    vec4 worldFar = invViewProj * clipFar;
    
    vec3 nearPoint = worldNear.xyz / worldNear.w;
    vec3 farPoint = worldFar.xyz / worldFar.w;
    
    vec3 rayDir = normalize(farPoint - nearPoint);
    
    float t = -nearPoint.y / rayDir.y;
    
    if (t < 0.0) {
        return vec3(9999.0, 0.0, 9999.0);
    }

    return nearPoint + t * rayDir;
}

float grid(vec2 coord, float spacing) {
    vec2 grid = abs(fract(coord / spacing - 0.5) - 0.5) / fwidth(coord / spacing);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main() {

    vec3 worldPos = worldPosFromScreenSpace(inUV);
    
    if (worldPos.x > 500.0 || worldPos.z > 500.0 || abs(worldPos.x) > 1000.0 || abs(worldPos.z) > 1000.0) {
        discard;
    }
    
    if (abs(worldPos.y) > 1.0) {
        discard;
    }
    
    vec2 gridCoord = worldPos.xz;
    vec2 gridLines = fract(gridCoord);

    bool onGridX = gridLines.x < 0.002 || gridLines.x > 0.998;
    bool onGridZ = gridLines.y < 0.002 || gridLines.y > 0.998;
    
    bool onXAxis = abs(worldPos.z) < 0.005; 
    bool onZAxis = abs(worldPos.x) < 0.005; 
    
    float distance = length(worldPos);
    if (distance > 50.0) {
        discard;
    }
    
    if (onXAxis) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0); 
    } else if (onZAxis) {
        outColor = vec4(0.0, 0.0, 1.0, 1.0); 
    } else if (onGridX || onGridZ) {
        outColor = vec4(0.45, 0.45, 0.45, 1.0); 
    } else {
        discard; 
    }
}