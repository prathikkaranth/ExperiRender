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
    
    if (compositorData.useRayTracer == 1) {
        // Use ray traced image
        outColor = vec4(rayTraced, 1.0);
    }
    else {
        // Use rasterized image
        outColor = vec4(rasterized, 1.0);
    }
}