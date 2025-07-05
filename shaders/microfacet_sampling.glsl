#ifndef MICROFACET_SAMPLING_GLSL
#define MICROFACET_SAMPLING_GLSL

// Note: random.glsl functions are expected to be available from the including shader

// Constants (PI should be defined elsewhere)
// const float PI = 3.14159265359;

// Convert roughness to alpha parameter for GGX
float roughness_to_alpha(float roughness) {
    return roughness * roughness;
}

// Sample GGX microfacet normal in tangent space
vec3 sample_ggx_normal(vec2 u, float alpha) {
    float cos_theta = sqrt((1.0 - u.x) / (u.x * (alpha * alpha - 1.0) + 1.0));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float phi = 2.0 * PI * u.y;
    
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

// GGX NDF
float ggx_distribution(float cos_theta_h, float alpha) {
    float alpha2 = alpha * alpha;
    float cos2 = cos_theta_h * cos_theta_h;
    float denom = cos2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

// Smith masking function for GGX
float smith_g1(float cos_theta, float alpha) {
    if (cos_theta <= 0.0) return 0.0;
    
    float alpha2 = alpha * alpha;
    float cos2 = cos_theta * cos_theta;
    float tan2 = (1.0 - cos2) / cos2;
    
    return 2.0 / (1.0 + sqrt(1.0 + alpha2 * tan2));
}

// Smith masking-shadowing function
float smith_g2(float cos_theta_i, float cos_theta_o, float alpha) {
    return smith_g1(cos_theta_i, alpha) * smith_g1(cos_theta_o, alpha);
}

// Create orthonormal basis from normal
void create_coordinate_system(vec3 N, out vec3 Nt, out vec3 Nb) {
    if (abs(N.z) < 0.999) {
        Nt = normalize(cross(vec3(0, 0, 1), N));
    } else {
        Nt = normalize(cross(vec3(1, 0, 0), N));
    }
    Nb = cross(N, Nt);
}

// Transform vector from tangent space to world space
vec3 tangent_to_world(vec3 v, vec3 N, vec3 Nt, vec3 Nb) {
    return v.x * Nt + v.y * Nb + v.z * N;
}

// Sample microfacet BRDF direction
struct MicrofacetSample {
    vec3 direction;
    float pdf;
    vec3 brdf_value;
};

MicrofacetSample sample_microfacet_brdf(
    vec3 wo,           // Outgoing direction (towards camera)
    vec3 N,            // Surface normal
    float roughness,   // Material roughness
    float metallic,    // Material metallic factor
    vec3 albedo,       // Material albedo
    inout uint seed
) {
    MicrofacetSample result;
    
    float alpha = roughness_to_alpha(roughness);
    
    // Create coordinate system
    vec3 Nt, Nb;
    create_coordinate_system(N, Nt, Nb);
    
    // Transform outgoing direction to tangent space
    vec3 wo_local = vec3(dot(wo, Nt), dot(wo, Nb), dot(wo, N));
    
    if (wo_local.z <= 0.0) {
        // Below surface
        result.direction = vec3(0.0);
        result.pdf = 0.0;
        result.brdf_value = vec3(0.0);
        return result;
    }
    
    // Sample microfacet normal
    vec2 u = vec2(rand1d(seed), rand1d(seed));
    vec3 h_local = sample_ggx_normal(u, alpha);
    
    // Reflect outgoing direction around microfacet normal
    vec3 wi_local = reflect(-wo_local, h_local);
    
    if (wi_local.z <= 0.0) {
        // Below surface
        result.direction = vec3(0.0);
        result.pdf = 0.0;
        result.brdf_value = vec3(0.0);
        return result;
    }
    
    // Transform back to world space
    result.direction = tangent_to_world(wi_local, N, Nt, Nb);
    
    // Calculate terms
    float cos_theta_i = wi_local.z;
    float cos_theta_o = wo_local.z;
    float cos_theta_h = h_local.z;
    float cos_theta_d = dot(wo_local, h_local); // Same as dot(wi_local, h_local)
    
    // Fresnel term (Schlick approximation)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cos_theta_d, 5.0);
    
    // Distribution term
    float D = ggx_distribution(cos_theta_h, alpha);
    
    // Geometry term
    float G = smith_g2(cos_theta_i, cos_theta_o, alpha);
    
    // BRDF value
    vec3 specular = F * D * G / (4.0 * cos_theta_i * cos_theta_o);
    
    // Lambertian diffuse term
    vec3 diffuse = albedo / PI * (1.0 - F) * (1.0 - metallic);
    
    result.brdf_value = diffuse + specular;
    
    // PDF calculation
    // p(h) = D * cos_theta_h
    float pdf_h = D * cos_theta_h;
    // Transform from half-vector to incident direction
    // p(wi) = p(h) / (4 * dot(wi, h))
    result.pdf = pdf_h / (4.0 * cos_theta_d);
    
    return result;
}

// Alternative: Sample diffuse + specular with multiple importance sampling
MicrofacetSample sample_microfacet_mis(
    vec3 wo,
    vec3 N,
    float roughness,
    float metallic,
    vec3 albedo,
    inout uint seed
) {
    MicrofacetSample result;
    
    // Determine sampling strategy based on material properties
    float specular_weight = mix(0.1, 0.9, 1.0 - roughness); // More specular for low roughness
    specular_weight = mix(specular_weight, 0.9, metallic);   // More specular for metals
    
    if (rand1d(seed) < specular_weight) {
        // Sample specular lobe
        result = sample_microfacet_brdf(wo, N, roughness, metallic, albedo, seed);
        // Adjust for sampling probability
        if (result.pdf > 0.0) {
            result.pdf *= specular_weight;
            result.brdf_value /= specular_weight;
        }
    } else {
        // Sample diffuse (cosine-weighted hemisphere)
        vec3 wi = cosine_weighted_hemisphere_sample(N, seed);
        
        result.direction = wi;
        result.pdf = (1.0 - specular_weight) * max(dot(wi, N), 0.0) / PI;
        
        // Calculate BRDF value for sampled direction
        float cos_theta_i = max(dot(wi, N), 0.0);
        float cos_theta_o = max(dot(wo, N), 0.0);
        
        if (cos_theta_i > 0.0 && cos_theta_o > 0.0) {
            vec3 h = normalize(wi + wo);
            float cos_theta_h = max(dot(N, h), 0.0);
            float cos_theta_d = max(dot(wo, h), 0.0);
            
            float alpha = roughness_to_alpha(roughness);
            
            // Fresnel
            vec3 F0 = mix(vec3(0.04), albedo, metallic);
            vec3 F = F0 + (1.0 - F0) * pow(1.0 - cos_theta_d, 5.0);
            
            // Distribution and Geometry
            float D = ggx_distribution(cos_theta_h, alpha);
            float G = smith_g2(cos_theta_i, cos_theta_o, alpha);
            
            vec3 specular = F * D * G / (4.0 * cos_theta_i * cos_theta_o);
            vec3 diffuse = albedo / PI * (1.0 - F) * (1.0 - metallic);
            
            result.brdf_value = (diffuse + specular) / (1.0 - specular_weight);
        } else {
            result.brdf_value = vec3(0.0);
            result.pdf = 0.0;
        }
    }
    
    return result;
}

#endif // MICROFACET_SAMPLING_GLSL