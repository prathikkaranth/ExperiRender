#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;


void main()
{
	prd.isHit = false;

	// Set strength to zero so this miss doesn't contribute to lighting
  	prd.strength = vec3(0.0);
}