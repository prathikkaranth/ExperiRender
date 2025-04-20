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

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT hitPayload prd;
hitAttributeEXT vec2 attribs;

const float PI = 3.14159265359f;

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
};

struct HitPoint {
  vec3 normal;
  vec2 uv;
};

layout(buffer_reference, std430) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference,  std430) buffer Indices {Index i[]; }; // Triangle indices
layout(set = 1, binding = 0, std430) buffer ObjDesc_ { 
    ObjDesc i[]; 
} m_objDesc;

layout(set = 2, binding = 0, std140) buffer MaterialsBuffer {
    MaterialRTData m[];
}
u_materials;

layout(set = 3, binding = 0) uniform sampler2D textures[];
layout(set = 3, binding = 1) uniform sampler2D normalMaps[];

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

  // Transform normal from [0,1] to [-1,1]
  normalTex = normalize(normalTex * 2.0 - 1.0);

  // Transform from tangent space to world space
  normal = normalize(TBN * normalTex);

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
    const float THRESHOLD = 1e-4f;
    return max(max(strength.r, strength.g), strength.b) < THRESHOLD;
}

vec3 compute_lambertian(in HitPoint hit_point) {
  const MaterialRTData material = u_materials.m[gl_InstanceCustomIndexEXT];

  const vec4 diffuseSample = texture(textures[material.albedoTexIndex], hit_point.uv);
  const vec3 diffuseColor = diffuseSample.rgb * material.albedo.rgb;

  vec3 lightDir = normalize(vec3(pcRay.lightPosition - gl_WorldRayOriginEXT));
  float diff = max(dot(hit_point.normal, lightDir), 0.0);

  // Light color and intensity
  vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light
  float lightIntensity = pcRay.lightIntensity;

  // Diffuse color
  return (diff * lightIntensity * lightColor * diffuseColor);
}

vec3 compute_directional_light_contribution(const vec3 normal, const vec3 next_origin, const vec3 diffuse_color)
{
    const vec3 light_dir = -normalize(pcRay.lightPosition); // Direction *from* surface point *to* light

    rayQueryEXT rq;
    const float tmin = 0.1f;
    rayQueryInitializeEXT(rq, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, next_origin, tmin, light_dir, 3000.0f);
    rayQueryProceedEXT(rq);
    
    if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT)
    {
        const float NdotL = max(dot(normal, light_dir), 0.0f);
        const vec3 light_color = vec3(1.f); // Light color
        
        // Multiply by diffuse color to respect material properties
        const vec3 diffuse = NdotL * light_color * diffuse_color;
        
        return diffuse * (pcRay.lightIntensity * 0.25);
    }

    return vec3(0.f); // in shadow
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

  // Compute the color
  vec3 vertex_color = compute_vert_color();
  const vec3 diffuse_color = compute_diffuse(hit_point);
  
  // Combine material colors
  vec3 material_color = diffuse_color * vertex_color;
  
  prd.next_direction = random_in_hemisphere(hit_point.normal, prd.seed);
  // Use a larger offset factor that scales with hit distance
  float offsetFactor = gl_HitTEXT * 0.001; // 0.1% of hit distance
  prd.next_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + hit_point.normal * max(0.01, offsetFactor);

  // Pass the material color to the light calculation
  prd.color += prd.strength * compute_directional_light_contribution(hit_point.normal, prd.next_origin, material_color);

  const float HEMISPHERE_PDF = 1.0f / (2.0f * PI);
  const float cos_theta = max(dot(hit_point.normal, prd.next_direction), 0.0f);
  prd.strength *= material_color * cos_theta / HEMISPHERE_PDF;

  if (is_strength_weak(prd.strength))
  {
    prd.next_direction = vec3(0.0f);
  }
}
