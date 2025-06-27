#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(set = 4, binding = 2) uniform sampler2D texSkybox;

void main()
{
	prd.isHit = false;

	vec3 dir = normalize(gl_WorldRayDirectionEXT);

	// Convert direction to spherical coordinates
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
	uv *= invAtan;
	uv += 0.5;

	// Sample the HDRI texture
	vec4 texColor = texture(texSkybox, uv);

	vec3 hdrColor = texColor.rgb;
	
	prd.color += prd.strength * hdrColor;
}