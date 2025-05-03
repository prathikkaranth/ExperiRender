struct hitPayload
{
    vec3 color;
    vec3 next_origin;
    vec3 next_direction;
    uint seed;
    vec3 strength;
	bool isHit;
};

// Push constant structure for the ray tracer
struct PushConstantRay
{
	uint seed;
	uint samples_done;
	uint depth;
};