#include "pbr_util.glsl"

// ----------------------------------------------------------------------------
// Complete microfacet BRDF function with enhanced features
vec3 MicrofacetBRDF(vec3 L, vec3 V, vec3 N, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Compute F0 (base reflectivity)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // Use enhanced BRDF terms
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmithCorrelated(N, V, L, roughness); // Enhanced geometry function
    vec3 F = fresnelSchlickRoughness(VdotH, F0, roughness); // Enhanced Fresnel with roughness
    
    // Compute specular term
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001; // Add epsilon to prevent division by zero
    vec3 specular = numerator / denominator;
    
    // Enhanced diffuse computation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    // Use Oren-Nayar for rough diffuse surfaces, Lambert for smooth ones
    vec3 diffuse;
    if (roughness > 0.3) {
        diffuse = OrenNayarDiffuse(L, V, N, albedo, roughness) * kD;
    } else {
        diffuse = kD * albedo / PI; // Standard Lambert for smooth surfaces
    }
    
    return (diffuse + specular) * NdotL;
}

vec3 BSDF(const float metallic, const float roughness, const vec3 normal, const vec3 view_dir, const vec3 light_dir, const vec3 diffuse_color) {
    
    const float NdotL = max(dot(normal, light_dir), 0.0f);
    // Use actual scene light color instead of hardcoded white
    const vec3 light_color = sceneData.sunlightColor.rgb;
    
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