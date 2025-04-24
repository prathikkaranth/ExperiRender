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

vec3 cosine_weighted_hemisphere_sample(vec3 normal, inout uint seed)
{
    // Generate random variables for spherical coordinates
    float r1 = rand01(seed);
    float r2 = rand01(seed);
    
    // Compute spherical coordinates with cosine weighting
    float phi = 2.0 * 3.14159265359 * r1;
    float cos_theta = sqrt(r2);  // This creates the cosine distribution
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    
    // Compute direction in local coordinate system
    vec3 direction;
    direction.x = cos(phi) * sin_theta;
    direction.y = sin(phi) * sin_theta;
    direction.z = cos_theta;
    
    // Create a coordinate system based on the normal
    vec3 up_vector = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up_vector, normal));
    vec3 bitangent = cross(normal, tangent);
    
    // Transform direction to world space
    mat3 TBN = mat3(tangent, bitangent, normal);
    return normalize(TBN * direction);
}