#version 450

layout (location = 0) out vec2 outUV;

void main() 
{
    // Create a fullscreen triangle using gl_VertexIndex [0, 1, 2]
    // These positions cover the screen with a large triangle
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    this will not compile
    // Convert from NDC (-1 to 1) to UV (0 to 1)
    outUV = positions[gl_VertexIndex] * 0.5 + 0.5;
}