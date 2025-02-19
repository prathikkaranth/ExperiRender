struct hitPayload
{
    vec3 color;
    vec3 next_origin;
    vec3 next_direction;
    uint seed;
    vec3 strength;
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
	uint seed;
	uint samples_done;
	uint depth;
};