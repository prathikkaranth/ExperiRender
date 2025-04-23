#include "pbr_util.glsl"

vec3 BSDF(const float metallic, const float roughness, const vec3 normal, const vec3 view_dir, const vec3 light_dir, const vec3 diffuse_color) {
    
    const float NdotL = max(dot(normal, light_dir), 0.0f);
    const vec3 light_color = vec3(1.f);
    
    // PBR calculations
    vec3 half_vec = normalize(light_dir + view_dir);
    
    // Calculate base reflectivity for metals vs non-metals
    vec3 F0 = vec3(0.04); // Base reflectivity for dielectrics
    F0 = mix(F0, diffuse_color, metallic); // Metals use albedo color for base reflectivity
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, half_vec, roughness);
    float G = GeometrySmith(normal, view_dir, light_dir, roughness);
    vec3 F = fresnelSchlick(max(dot(half_vec, view_dir), 0.0), F0);
    
    vec3 kS = F; // Specular contribution
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic); // Diffuse contribution (metallic surfaces don't diffuse)
    
    // Combine specular components
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, view_dir), 0.0) * NdotL + 0.0001; // Prevent division by zero
    vec3 specular = numerator / denominator;
    
    // Combine diffuse and specular for final result
    vec3 bsdf = (kD * diffuse_color / PI + specular) * NdotL * light_color;
    
    return bsdf;
}