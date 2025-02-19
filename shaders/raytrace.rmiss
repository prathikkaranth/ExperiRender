#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(push_constant) uniform _PushConstantRay
{
  PushConstantRay pcRay;
};

void main()
{
  // Sky color
	const float t = 0.5*(normalize(gl_WorldRayDirectionEXT).y + 1);
	const vec3 skyColor = mix(vec3(1.0), vec3(0.702, 0.824, 1), t);

	prd.color = (vec3(1.0f) * 5.0f) * prd.strength;

}