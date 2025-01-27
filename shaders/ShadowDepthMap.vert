#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

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

	gl_Position = sceneData.lightSpaceMatrix * worldPos;

    // gl_Position = sceneData.lightSpaceMatrix * PushConstants.render_matrix * vec4(v.position, 1.0);
}