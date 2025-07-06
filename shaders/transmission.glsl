#ifndef TRANSMISSION_GLSL
#define TRANSMISSION_GLSL

// Calculates Fresnel reflectance using Schlick's approximation
float fresnel_schlick(float cos_theta, float f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

// Calculates Fresnel reflectance using full Fresnel equations
float fresnel_exact(float cos_i, float cos_t, float n1, float n2) {
    float rs = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t);
    float rp = (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t);
    return 0.5 * (rs * rs + rp * rp);
}

// Returns refracted direction, or vec3(0) if total internal reflection occurs
vec3 refract_ray(vec3 incident, vec3 normal, float eta, out bool total_internal_reflection) {
    vec3 refracted = refract(incident, normal, eta);
    
    // GLSL refract() returns vec3(0) for total internal reflection
    total_internal_reflection = (refracted == vec3(0.0));
    
    return refracted;
}

// Adds roughness-based scattering to a direction
vec3 add_roughness_scattering(vec3 direction, vec3 surface_normal, float roughness, inout uint seed) {
    if (roughness <= 0.05) {
        return direction;
    }
    
    // Generate scatter vector in hemisphere around the direction
    vec3 scatter = cosine_weighted_hemisphere_sample(direction, seed) * roughness * 0.3;
    return normalize(direction + scatter);
}

// Main transmission calculation function
// Returns: next ray direction, origin offset, and attenuation factor
struct TransmissionResult {
    vec3 direction;
    vec3 origin_offset;
    vec3 attenuation;
    bool terminate_ray;
};

TransmissionResult calculate_transmission(
    vec3 incident_dir,
    vec3 hit_normal,
    vec3 hit_position,
    float material_ior,
    float transmission_factor,
    float roughness,
    vec3 material_color,
    vec3 ray_strength,
    inout uint seed
) {
    TransmissionResult result;
    result.terminate_ray = false;
    
    const float air_ior = 1.0;
    
    vec3 incident = normalize(incident_dir);
    vec3 normal = hit_normal;
    
    // Determine if we're entering or exiting the material
    bool entering = dot(incident, normal) < 0.0;
    if (!entering) {
        normal = -normal; // Flip normal for exiting rays
    }
    
    float eta = entering ? (air_ior / material_ior) : (material_ior / air_ior);
    float cos_i = abs(dot(incident, normal));
    
    // Calculate refraction and check for total internal reflection
    bool total_internal_reflection;
    vec3 refracted_dir = refract_ray(incident, normal, eta, total_internal_reflection);
    
    float fresnel_reflectance;
    if (total_internal_reflection) {
        fresnel_reflectance = 1.0;
    } else {
        // Calculate Fresnel reflectance
        float f0 = pow((air_ior - material_ior) / (air_ior + material_ior), 2.0);
        fresnel_reflectance = fresnel_schlick(cos_i, f0);
    }
    
    float rand_choice = rand1d(seed);
    
    if (total_internal_reflection) {
        // Total internal reflection - ray must reflect internally
        vec3 reflected_dir = reflect(incident, normal);
        reflected_dir = add_roughness_scattering(reflected_dir, normal, roughness * 0.5, seed);
        
        result.direction = reflected_dir;
        
        // Stay inside the material
        if (entering) {
            result.origin_offset = -normal * 1e-4; // Stay inside
        } else {
            result.origin_offset = normal * 1e-4;  // Stay inside from other side
        }
        
        // Internal reflections maintain most energy
        result.attenuation = material_color * 0.95;
        
    } else {
        // No total internal reflection - choose between transmission and reflection
        float effective_transmission = transmission_factor * (1.0 - fresnel_reflectance);
        
        // Roughness reduces transmission probability
        effective_transmission *= (1.0 - roughness * 0.6);
        
        if (rand_choice < effective_transmission) {
            // Transmission with refraction
            refracted_dir = add_roughness_scattering(refracted_dir, refracted_dir, roughness, seed);
            
            result.direction = refracted_dir;
            
            // Move to the other side of the interface
            if (entering) {
                result.origin_offset = -normal * 1e-4; // Enter material
            } else {
                result.origin_offset = normal * 1e-4;  // Exit material
            }
            
            // Transmission strength accounting for Fresnel loss
            result.attenuation = material_color * (1.0 - fresnel_reflectance);
            
        } else {
            // Fresnel reflection (external reflection)
            vec3 reflected_dir = reflect(incident, normal);
            reflected_dir = add_roughness_scattering(reflected_dir, normal, roughness, seed);
            
            result.direction = reflected_dir;
            
            // External reflection - stay on the side we came from
            if (entering) {
                result.origin_offset = normal * 1e-4;  // Reflect back to air
            } else {
                result.origin_offset = -normal * 1e-4; // Reflect back into material
            }
            
            result.attenuation = material_color * fresnel_reflectance;
        }
    }
    
    return result;
}

#endif // TRANSMISSION_GLSL