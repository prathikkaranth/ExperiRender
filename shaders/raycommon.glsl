struct hitPayload
{
    vec3 color;
    vec3 next_origin;
    vec3 next_direction;
    uint seed;
    vec3 strength;
	bool isHit;
};

struct PushConstantRay
{
	uint seed;
	uint samples_done;
	uint depth;
	uint useMicrofacetSampling;
};