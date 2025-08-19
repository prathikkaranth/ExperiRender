/*
 Implementation of FXAA (Fast Approximate Anti-Aliasing) based on the algorithm described by Timothy Lottes.
 https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
*/

#version 450

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D R_filterTexture;

layout(set = 0, binding = 1) uniform FXAAData {   
    vec3 R_inverseFilterTextureSize;
    float R_fxaaSpanMax;
    float R_fxaaReduceMin;
    float R_fxaaReduceMul;
} fxaaData;

void main()
{
    vec2 texCoordOffset = fxaaData.R_inverseFilterTextureSize.xy;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);    
    float lumaTL = dot(luma, texture(R_filterTexture, inUV + (vec2(-1.0, -1.0) * texCoordOffset)).xyz);
    float lumaTR = dot(luma, texture(R_filterTexture, inUV + (vec2(1.0, -1.0) * texCoordOffset)).xyz);
    float lumaBL = dot(luma, texture(R_filterTexture, inUV + (vec2(-1.0, 1.0) * texCoordOffset)).xyz);
    float lumaBR = dot(luma, texture(R_filterTexture, inUV + (vec2(1.0, 1.0) * texCoordOffset)).xyz);
    float lumaM  = dot(luma, texture(R_filterTexture, inUV).xyz);

    vec2 dir;
    dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
    dir.y = ((lumaTL + lumaBL) - (lumaTR + lumaBR));
    
    float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (fxaaData.R_fxaaReduceMul * 0.25), fxaaData.R_fxaaReduceMin);
    float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(fxaaData.R_fxaaSpanMax, fxaaData.R_fxaaSpanMax), 
        max(vec2(-fxaaData.R_fxaaSpanMax, -fxaaData.R_fxaaSpanMax), dir * inverseDirAdjustment)) * texCoordOffset;

    vec3 result1 = (1.0/2.0) * (
        texture(R_filterTexture, inUV + (dir * vec2(1.0/3.0 - 0.5))).xyz +
        texture(R_filterTexture, inUV + (dir * vec2(2.0/3.0 - 0.5))).xyz);

    vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (
        texture(R_filterTexture, inUV + (dir * vec2(0.0/3.0 - 0.5))).xyz +
        texture(R_filterTexture, inUV + (dir * vec2(3.0/3.0 - 0.5))).xyz);

    float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
    float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
    float lumaResult2 = dot(luma, result2);
    
    if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
        outColor = vec4(result1, 1.0);
    else
        outColor = vec4(result2, 1.0);
}