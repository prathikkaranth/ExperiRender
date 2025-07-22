#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_ray_query : require
#include "raycommon.glsl"
#include "random.glsl"
#include "input_structures.glsl"
#include "PBRMetallicRoughness.glsl"
#include "transmission.glsl"
#include "microfacet_sampling.glsl"

layout(binding = 0, set = 1) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT hitPayload prd;
hitAttributeEXT vec2 attribs;

struct Vertex {
  vec3 position;
  float uv_x;
  vec3 normal;
  float uv_y;
  vec4 color;
  vec3 tangent;
  vec3 bitangent;
};

struct ObjDesc {
  uint64_t vertexAddress;         
	uint64_t indexAddress;   
  uint firstIndex;
  uint matIndex;       
};

struct Index {
  uint elems[3];
};

struct MaterialRTData {
  vec4 albedo;
  uint albedoTexIndex;
  uint p0;
  uint p1;
  uint p2;
  vec4 metal_rough_factors;
  float transmissionFactor;
  uint hasTransmissionTex;
  float ior;
  uint hasEmissiveTex;
  vec4 emissiveFactor;
};

struct HitPoint {
  vec3 normal;
  vec2 uv;
};

layout(buffer_reference, std430) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference,  std430) buffer Indices {Index i[]; }; // Triangle indices
layout(set = 2, binding = 0, std430) buffer ObjDesc_ { 
    ObjDesc i[]; 
} m_objDesc;

layout(set = 3, binding = 0, std140) buffer MaterialsBuffer {
    MaterialRTData m[];
}
u_materials;

layout(set = 4, binding = 0) uniform sampler2D textures[];
layout(set = 4, binding = 1) uniform sampler2D normalMaps[];
layout(set = 4, binding = 2) uniform sampler2D texSkybox;
layout(set = 4, binding = 3) uniform sampler2D metalRoughMaps[];
layout(set = 4, binding = 4) uniform sampler2D emissiveMaps[];

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

HitPoint compute_hit_point() {
  // Object Data
  ObjDesc objResource  = m_objDesc.i[gl_InstanceCustomIndexEXT];
  Indices    indices   = Indices(objResource.indexAddress + objResource.firstIndex * 4);
  Vertices   vertices  = Vertices(objResource.vertexAddress);

  // Indices of the triangle
  uvec3 ind = uvec3(
    indices.i[gl_PrimitiveID].elems[0],
    indices.i[gl_PrimitiveID].elems[1],
    indices.i[gl_PrimitiveID].elems[2]);

  // Vertex of the triangle
  Vertex v0 = vertices.v[ind.x];
  Vertex v1 = vertices.v[ind.y];
  Vertex v2 = vertices.v[ind.z];

  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Interpolate normal, tangent, and bitangent
  vec3 normal   = normalize(v0.normal   * barycentrics.x + v1.normal   * barycentrics.y + v2.normal   * barycentrics.z);
  vec3 tangent  = normalize(v0.tangent  * barycentrics.x + v1.tangent  * barycentrics.y + v2.tangent  * barycentrics.z);
  vec3 bitangent = normalize(v0.bitangent * barycentrics.x + v1.bitangent * barycentrics.y + v2.bitangent * barycentrics.z);

  // Construct TBN matrix
  mat3 TBN = mat3(tangent, bitangent, normal);

  // Compute UV coordinates
  vec2 uv = vec2(v0.uv_x * barycentrics.x + v1.uv_x * barycentrics.y + v2.uv_x * barycentrics.z,
                 v0.uv_y * barycentrics.x + v1.uv_y * barycentrics.y + v2.uv_y * barycentrics.z);

  // Fetch normal map index from material
  MaterialRTData material = u_materials.m[gl_InstanceCustomIndexEXT];
  vec3 normalTex = texture(normalMaps[material.albedoTexIndex], uv).rgb;

  // Check if this is a real normal map vs default texture
  // Default textures are often white (1,1,1) or neutral grey (~0.5,0.5,1 in normal map space)
  bool hasRealNormalMap = false;
  
  // Check for default white texture (1,1,1)
  if (length(normalTex - vec3(1.0)) > 0.1) {
    // Check for default neutral normal (0.5,0.5,1) -> (0,0,1) in tangent space
    vec3 neutralNormal = vec3(0.5, 0.5, 1.0);
    if (length(normalTex - neutralNormal) > 0.1) {
      // Check for default grey texture (0.66, 0.66, 0.66)
      if (length(normalTex - vec3(0.66)) > 0.1) {
        hasRealNormalMap = true;
      }
    }
  }

  if (hasRealNormalMap) {
    // Transform normal from [0,1] to [-1,1]
    normalTex = normalize(normalTex * 2.0 - 1.0);
    // Transform from tangent space to world space
    normal = normalize(TBN * normalTex);
  }
  // Otherwise, just use the interpolated vertex normal

  return HitPoint(normal, uv);
}


vec3 compute_diffuse(in HitPoint hit_point) {

  const MaterialRTData material = u_materials.m[gl_InstanceCustomIndexEXT];

  const vec4 diffuseSample = texture(textures[material.albedoTexIndex], hit_point.uv);
  const vec3 diffuseColor = diffuseSample.rgb * material.albedo.rgb;

  return diffuseColor;
}

bool is_strength_weak(vec3 strength)
{
    const float THRESHOLD = 1e-3f;  // More aggressive termination
    float max_strength = max(max(strength.r, strength.g), strength.b);
    // Also terminate if strength becomes too large (prevents accumulation of huge values)
    return max_strength < THRESHOLD || max_strength > 100.0f;
}

vec3 compute_light_contribution(const vec3 normal, const vec3 surface_pos, const vec3 light_dir, const vec3 light_color, const vec3 diffuse_color, const float metalness, const float roughness, const bool do_shadow_test)
{
    const vec3 view_dir = normalize(gl_WorldRayOriginEXT - surface_pos); // Direction to camera/viewer
    
    // Optional shadow test
    if (do_shadow_test) {
        rayQueryEXT rq;
        const float tmin = 0.01f;
        rayQueryInitializeEXT(rq, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, surface_pos, tmin, light_dir, 3000.0f);
        rayQueryProceedEXT(rq);
        
        if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            return vec3(0.f); // in shadow
        }
    }
    
    // Compute the BSDF using the PBR model
    vec3 bsdf = BSDF(metalness, roughness, normal, view_dir, light_dir, diffuse_color);
    
    return bsdf * light_color;
}

vec3 compute_directional_light_contribution(const vec3 normal, const vec3 next_origin, const vec3 diffuse_color, const vec2 uv, const float metalness, const float roughness)
{
    const vec3 light_dir = -normalize(sceneData.sunlightDirection.xyz); // Direction *from* surface point *to* light
    const vec3 light_color = sceneData.sunlightDirection.w * sceneData.sunlightColor.rgb;
    
    return compute_light_contribution(normal, next_origin, light_dir, light_color, diffuse_color, metalness, roughness, true);
}

vec3 compute_point_light_contribution(const vec3 normal, const vec3 next_origin, const vec3 diffuse_color, const vec2 uv, const float metalness, const float roughness)
{
    const vec3 light_pos = sceneData.pointLightPosition.xyz;
    const vec3 light_dir = normalize(light_pos - next_origin); // Direction *from* surface point *to* light
    const float light_distance = length(light_pos - next_origin);
    
    // Point light attenuation
    const float range = sceneData.pointLightPosition.w;
    const float attenuation = 1.0 / (1.0 + light_distance * light_distance / (range * range));
    
    const vec3 light_color = sceneData.pointLightColor.w * sceneData.pointLightColor.rgb * attenuation;
    
    return compute_light_contribution(normal, next_origin, light_dir, light_color, diffuse_color, metalness, roughness, true);
}


vec3 compute_vert_color() {
  // Object Data
  ObjDesc objResource  = m_objDesc.i[gl_InstanceCustomIndexEXT];
  Indices    indices     = Indices(objResource.indexAddress + objResource.firstIndex * 4);
  Vertices   vertices    = Vertices(objResource.vertexAddress);

  // Indices of the triangle
  uvec3 ind = ivec3(
    indices.i[gl_PrimitiveID].elems[0],
    indices.i[gl_PrimitiveID].elems[1],
    indices.i[gl_PrimitiveID].elems[2]);
  
  // Vertex of the triangle
  Vertex v0 = vertices.v[ind.x];
  Vertex v1 = vertices.v[ind.y];
  Vertex v2 = vertices.v[ind.z];

  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Computing the color at hit position
  vec4 color = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;
  // Transforming the color to world space
  return color.rgb;
}

void main()
{
  // Compute the hitpoint
    const HitPoint hit_point = compute_hit_point();

    // Get the material data
    const MaterialRTData material = u_materials.m[gl_InstanceCustomIndexEXT];

    const vec4 diffuseSample = texture(textures[material.albedoTexIndex], hit_point.uv);
    const float alpha = diffuseSample.a;

    if (alpha < 0.01f) {
      // Continue the ray in same direction, nudging out to avoid self-hit
      vec3 nudge = dot(gl_WorldRayDirectionEXT, hit_point.normal) > 0.0
          ? hit_point.normal * 1e-4f   // Front face
          : -hit_point.normal * 1e-4f; // Back face

      prd.next_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + nudge;
      prd.next_direction = gl_WorldRayDirectionEXT;

      // Keep strength the same — this is a transparent skip
    return;
    }
    
    // Default roughness and metalness values
    float roughness = 0.5;
    float metalness = 0.0;

    vec3 metalRoughSample = texture(metalRoughMaps[material.albedoTexIndex], hit_point.uv).rgb;
    roughness = metalRoughSample.g * material.metal_rough_factors.y;
    metalness = metalRoughSample.b * material.metal_rough_factors.x;

    // Compute the color
    vec3 vertex_color = compute_vert_color();
    const vec3 diffuse_color = compute_diffuse(hit_point);
    
    // Combine material colors
    vec3 material_color = diffuse_color * vertex_color;
    
    // Sample emissive properties
    vec3 emissive_sample = texture(emissiveMaps[material.albedoTexIndex], hit_point.uv).rgb;
    vec3 emissive_color = vec3(0.0);
    
    // Check if emissive texture is NOT a default white/black texture
    bool isDefaultWhite = length(emissive_sample - vec3(1.0)) < 0.01; // Pure white
    bool isDefaultBlack = length(emissive_sample) < 0.01; // Pure black
    
    if (!isDefaultWhite && !isDefaultBlack) {
        // Use emissive texture multiplied by factor (real emissive content)
        emissive_color = emissive_sample * material.emissiveFactor.rgb;
    } else if (length(material.emissiveFactor.rgb) > 0.0) {
        // Only use emissive factor if texture is default and factor is non-zero
        emissive_color = material.emissiveFactor.rgb;
    }
    
    // For now, just display emissive content additively for visualization
    // This will be replaced with proper emissive lighting later
    if (length(emissive_color) > 0.0) {
        // Simple additive display of emissive content - no complex lighting
        material_color += emissive_color * 0.3; // Add to material color for display
    }
    
    // Compute direct lighting contribution 
    vec3 direct_light = compute_directional_light_contribution(hit_point.normal, 
                                                             gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + hit_point.normal * 0.001f, 
                                                             material_color, 
                                                             hit_point.uv,
                                                             metalness,
                                                             roughness);
    
    // Add point light contribution
    direct_light += compute_point_light_contribution(hit_point.normal, 
                                                   gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + hit_point.normal * 0.001f, 
                                                   material_color, 
                                                   hit_point.uv,
                                                   metalness,
                                                   roughness);
    
    // Fix overexposed lighting - scale down the direct lighting
    direct_light *= 0.1; // Scale down by 10x to fix overexposure
    
    // Add direct lighting contribution to the path radiance
    prd.color += prd.strength * direct_light;
    
    // Set up next ray origin
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    
    vec3 new_strength;
    
    // Handle transmission materials properly
    if (material.transmissionFactor > 0.0f) {
      TransmissionResult transmission = calculate_transmission(
        gl_WorldRayDirectionEXT,
        hit_point.normal,
        hit_position,
        material.ior,
        material.transmissionFactor,
        roughness,
        material_color,
        prd.strength,
        prd.seed
      );
      
      if (transmission.terminate_ray) {
        prd.next_direction = vec3(0.0f);
        return;
      }
      
      prd.next_direction = transmission.direction;
      prd.next_origin = hit_position + transmission.origin_offset;
      new_strength = prd.strength * transmission.attenuation;
    } else if (metalness > 0.99f && roughness < 0.05f) {
      // Mix between perfect reflection and BSDF sampling
      vec3 incident = gl_WorldRayDirectionEXT;
      vec3 perfect_reflection = reflect(incident, hit_point.normal);
      
      // Use perfect reflection direction
      prd.next_direction = perfect_reflection;
      
      // For reflection, offset ray origin away from surface
      prd.next_origin = hit_position + hit_point.normal * 1e-4f;
      
      // For perfect reflection, use simple attenuation
      new_strength = prd.strength * material_color;
    } else {
      if (pcRay.useMicrofacetSampling == 1) {
        // Decide sampling strategy based on material properties
        // Use microfacet sampling for specular materials, cosine hemisphere for diffuse
        float specular_factor = mix(metalness, 1.0 - roughness, 0.5); // Blend metalness and smoothness
        
        if (specular_factor > 0.3) {
          // Use microfacet sampling for specular materials
          vec3 wo = normalize(-gl_WorldRayDirectionEXT); // Direction towards camera
          
          MicrofacetSample brdf_sample = sample_microfacet_brdf(
            wo,
            hit_point.normal,
            roughness,
            metalness,
            material_color,
            prd.seed
          );
          
          if (brdf_sample.pdf <= 0.0 || any(isnan(brdf_sample.direction)) || any(isinf(brdf_sample.brdf_value))) {
            // Fallback to cosine hemisphere if microfacet fails
            prd.next_direction = cosine_weighted_hemisphere_sample(hit_point.normal, prd.seed);
            prd.next_origin = hit_position + hit_point.normal * 1e-4f;
            
            vec3 bsdf = BSDF(metalness, roughness, hit_point.normal, -gl_WorldRayDirectionEXT, prd.next_direction, material_color);
            const float cos_theta = max(dot(hit_point.normal, prd.next_direction), 0.0f);
            const float HEMISPHERE_PDF = 1.0f / (2.0f * PI);
            new_strength = prd.strength * bsdf * cos_theta / HEMISPHERE_PDF;
          } else {
            prd.next_direction = brdf_sample.direction;
            prd.next_origin = hit_position + hit_point.normal * 1e-4f;
            
            const float cos_theta = max(dot(hit_point.normal, brdf_sample.direction), 0.0f);
            vec3 contribution = brdf_sample.brdf_value * cos_theta / brdf_sample.pdf;
            contribution = min(contribution, vec3(10.0)); // Clamp to prevent fireflies
            new_strength = prd.strength * contribution;
          }
        } else {
          // Use cosine hemisphere sampling for diffuse materials
          prd.next_direction = cosine_weighted_hemisphere_sample(hit_point.normal, prd.seed);
          prd.next_origin = hit_position + hit_point.normal * 1e-4f;
          
          vec3 bsdf = BSDF(metalness, roughness, hit_point.normal, -gl_WorldRayDirectionEXT, prd.next_direction, material_color);
          const float cos_theta = max(dot(hit_point.normal, prd.next_direction), 0.0f);
          const float HEMISPHERE_PDF = 1.0f / (2.0f * PI);
          new_strength = prd.strength * bsdf * cos_theta / HEMISPHERE_PDF;
        }
      } else {
        // Fallback to cosine-weighted hemisphere sampling
        prd.next_direction = cosine_weighted_hemisphere_sample(hit_point.normal, prd.seed);
        
        // For regular BSDF, offset ray origin away from surface
        prd.next_origin = hit_position + hit_point.normal * 1e-4f;
        
        vec3 bsdf = BSDF(metalness, roughness, hit_point.normal, -gl_WorldRayDirectionEXT, prd.next_direction, material_color);
        
        // Update the path strength for the next bounce
        const float cos_theta = max(dot(hit_point.normal, prd.next_direction), 0.0f);
        const float HEMISPHERE_PDF = 1.0f / (2.0f * PI);
        
        // Update strength based on BRDF and PDF
        new_strength = prd.strength * bsdf * cos_theta / HEMISPHERE_PDF;
      }
    }
    
    // Safety checks to prevent NaN/infinite values
    if (any(isnan(new_strength)) || any(isinf(new_strength)) || any(lessThan(new_strength, vec3(0.0)))) {
        prd.next_direction = vec3(0.0f);
        return;
    }
    
    prd.strength = new_strength;

    // Poor man's russian roulette
    if (is_strength_weak(prd.strength))
      prd.next_direction = vec3(0.0f);
}
