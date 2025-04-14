#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(set = 3, binding = 2) uniform sampler2D texSkybox;

layout(push_constant) uniform _PushConstantRay
{
  PushConstantRay pcRay;
};


void main()
{
  // Sky color
	const float t = 0.5*(normalize(gl_WorldRayDirectionEXT).y + 1);
	const vec3 skyColor = mix(vec3(1.0), vec3(0.702, 0.824, 1), t);

	// Normalize ray direction
	vec3 dir = normalize(gl_WorldRayDirectionEXT);

	// Convert direction to spherical coordinates
	float u = 0.5 + atan(dir.z, dir.x) / (2.0 * 3.14159265359);
	float v = 0.5 - asin(dir.y) / 3.14159265359;

	// Flip the V coordinate to match the rasterizer's flipped texture
	v = 1.0 - v;

	// Sample the HDRI texture
	vec4 texColor = texture(texSkybox, vec2(u, v));

	// Apply only HDR color to the skybox
	vec3 hdrColor = texColor.rgb * texColor.a;

	// Tone mapping function (Simple Reinhard)
	float exposure = 1.0; // Adjust as needed
	hdrColor *= exposure;
	hdrColor = hdrColor / (hdrColor + vec3(1.0));

	// Gamma correction
	float gamma = 2.2; // Standard gamma value
	hdrColor = pow(hdrColor, vec3(1.0 / gamma));

	// Set the ray payload color to the sky color and the intensity of the light
	prd.color = pcRay.lightIntensity * hdrColor * prd.strength;

}