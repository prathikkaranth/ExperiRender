#version 450
#extension GL_GOOGLE_include_directive : require

#include "ACES.glsl"
#include "pbrNeutral.glsl"
#include "SmartDeNoise.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D rayTracedImage;
layout (set = 0, binding = 1) uniform sampler2D rasterizedImage;

layout(set = 0, binding = 2) uniform  CompositerData{   
    int useRayTracer;

    // Denoising parameters
    int useDenoiser; // 0 = no denoising, 1 = denoising
    float sigma; // Standard deviation for Gaussian filter
    float kSigma; // Standard deviation for kernel size
    float threshold; // Threshold for denoising
    
    float exposure;
} compositorData;

void main() 
{
    // Sample both images
    vec3 rasterized = texture(rasterizedImage, inUV).rgb;
    //vec3 rayTraced = texture(rayTracedImage, inUV).rgb;

    // Apply tone mapping to rasterized image as before
    rasterized = rasterized / (rasterized + vec3(1.0));
    rasterized = pow(rasterized, vec3(1.0/2.2)); 

    vec4 rayTraced = vec4(0.0);
    if (compositorData.useDenoiser == 1) {
       rayTraced = smartDeNoise(rayTracedImage, inUV, compositorData.sigma, compositorData.kSigma, compositorData.threshold);
    }
    else {
       rayTraced = texture(rayTracedImage, inUV);
    }

    // Apply exposure adjustment before ACES filmic tone mapping
    rayTraced *= compositorData.exposure;
    
    // Tone mapping for rayTraced
    rayTraced.rgb = ACESFitted(rayTraced.rgb);

    // Gamma correction for rayTraced
    rayTraced.rgb = pow(rayTraced.rgb, vec3(1.0/2.2));
    
    if (compositorData.useRayTracer == 1) {
        // Use ray traced image
        outColor = vec4(rayTraced);
    }
    else {
        // Use rasterized image
        outColor = vec4(rasterized, 1.0);
    }
}