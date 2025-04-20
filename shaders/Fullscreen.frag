#version 450

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D rayTracedImage;
layout (set = 0, binding = 1) uniform sampler2D rasterizedImage;

layout(set = 0, binding = 2) uniform  CompositerData{   
    int useRayTracer;
} compositorData;

void main() 
{
    // Sample both images
    vec3 rasterized = texture(rasterizedImage, inUV).rgb;
    vec3 rayTraced = texture(rayTracedImage, inUV).rgb;

    // Apply tone mapping to rasterized image as before
    rasterized = rasterized / (rasterized + vec3(1.0));
    rasterized = pow(rasterized, vec3(1.0/2.2)); 
    
    // For ray traced: Apply a more color-preserving tone mapper
    // Option 1: ACES filmic tone mapping (preserves more color saturation)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    rayTraced = clamp((rayTraced * (a * rayTraced + b)) / (rayTraced * (c * rayTraced + d) + e), 0.0, 1.0);
    
    // Or Option 2: Add a saturation adjustment after tone mapping
    /*
    rayTraced = rayTraced / (rayTraced + vec3(1.0));
    // Increase saturation
    float luminance = dot(rayTraced, vec3(0.2126, 0.7152, 0.0722));
    rayTraced = mix(vec3(luminance), rayTraced, 1.5); // 1.5 increases saturation
    */
    
    // Gamma correction for rayTraced
    rayTraced = pow(rayTraced, vec3(1.0/2.2));
    
    if (compositorData.useRayTracer == 1) {
        // Use ray traced image
        outColor = vec4(rayTraced, 1.0);
    }
    else {
        // Use rasterized image
        outColor = vec4(rasterized, 1.0);
    }
}