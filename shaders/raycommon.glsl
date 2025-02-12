struct hitPayload
{
  vec3 hitValue;
};

// Push constant structure for the ray tracer
struct PushConstantRay
{
  vec4  clearColor;
	vec3  lightPosition;
	mat4  viewInverse;
	mat4  projInverse;
	float lightIntensity;
	int   lightType;
};