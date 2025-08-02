#version 450

// Input from vertex shader
layout(location = 0) in vec3 inTexCoord;

// Output color
layout(location = 0) out vec4 outColor0;  // First attachment
layout(location = 1) out vec4 outColor1;  // Second attachment (_hdriOutImage)

// Cubemap sampler
layout(set = 0, binding = 0) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

// Simple Reinhard tone mapping function
vec3 toneMap(vec3 color, float exposure)
{
    color *= exposure;
    
    color = color / (color + vec3(1.0));
    
    return color;
}

// Gamma correction
vec3 gammaCorrect(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(inTexCoord)); 
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    outColor0 = vec4(color, 1.0);
    outColor1 = vec4(color, 1.0); // Assuming you want the same color for the second attachment
}