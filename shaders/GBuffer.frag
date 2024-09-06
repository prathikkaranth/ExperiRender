#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragWorldPos;
layout (location = 1) out vec4 outFragWorldNormal;

struct Empty{ float e; };

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	Empty e[];
} PushConstants;

void main() 
{
	// Normalized normal
	vec3 normal = normalize(inNormal);

	// G buffer World position
	outFragWorldPos = vec4(inWorldPos,1.0f);

	// G buffer World normal
	outFragWorldNormal = vec4(normal, 1.0f);
}