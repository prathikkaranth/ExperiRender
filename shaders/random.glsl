uint pcg_hash(inout uint seed)
{
    const uint state = seed;
    seed = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand01(inout uint seed)
{
    const uint rand_uint = pcg_hash(seed);
    return float(rand_uint) / float(0xFFFFFFFFu);
}

float rand1d(inout uint seed)
{
    return 2.0 * rand01(seed) - 1.0;
}

vec3 random_unit_vector(inout uint seed)
{
    while (true) {
        vec3 rand_dir = vec3(rand1d(seed), rand1d(seed), rand1d(seed));
        if (length(rand_dir) <= 1.0) {
            return normalize(rand_dir);
        }
    }
}

vec3 random_in_hemisphere(vec3 normal, inout uint seed)
{
    const vec3 rand_dir = random_unit_vector(seed);
    if (dot(rand_dir, normal) < 0.0) {
        return -rand_dir;
    }
    return rand_dir;
}