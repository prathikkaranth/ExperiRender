#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require


#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;
layout (location = 5) out vec3 outBitangent;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
	vec3 tangent;
	vec3 bitangent;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
	vec4 position = vec4(v.position, 1.0f);

	vec4 worldPos = PushConstants.render_matrix * position;

	gl_Position =  sceneData.viewproj * worldPos;
	// gl_Position =  PushConstants.render_matrix *position;
	
	mat4 invTransposeRenderMatrix = transpose(inverse(PushConstants.render_matrix));

	outNormal = (invTransposeRenderMatrix * vec4(v.normal, 0.f)).xyz;
	outColor = v.color.xyz * materialData.colorFactors.xyz;	
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
	outTangent = v.tangent;
	outBitangent = v.bitangent;


	outWorldPos = worldPos.xyz / worldPos.w;
}