#version 450
#extension GL_GOOGLE_include_directive : require

#include "ACES.glsl"
#include "pbrNeutral.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D rayTracedImage;
layout (set = 0, binding = 1) uniform sampler2D rasterizedImage;

layout(set = 0, binding = 2) uniform  CompositerData{   
    int useRayTracer;
    float exposure;
} compositorData;

void main() 
{
    vec3 rasterized = texture(rasterizedImage, inUV).rgb;

    // Apply tone mapping to rasterized image as before
    rasterized *= compositorData.exposure;
    rasterized = PBRNeutralToneMapping(rasterized);
    rasterized = pow(rasterized, vec3(1.0/2.2));

    vec4 rayTraced = texture(rayTracedImage, inUV);

    // Apply exposure adjustment and PBR Neutral tone mapping
    rayTraced *= compositorData.exposure;
    rayTraced.rgb = PBRNeutralToneMapping(rayTraced.rgb);
    rayTraced.rgb = pow(rayTraced.rgb, vec3(1.0/2.2));
    
    if (compositorData.useRayTracer == 1) {
        outColor = vec4(rayTraced);
    }
    else {
        outColor = vec4(rasterized, 1.0);
    }
}