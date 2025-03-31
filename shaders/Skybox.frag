#version 450

// Input from vertex shader
layout(location = 0) in vec3 inTexCoord;

// Output color
layout(location = 0) out vec4 outColor;

// Cubemap sampler
layout(set = 0, binding = 0) uniform samplerCube cubemapSampler;

void main() {
    // Sample the cubemap using the texture coordinates
    outColor = texture(cubemapSampler, inTexCoord);
}