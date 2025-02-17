#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : enable
#include "raycommon.glsl"
#include "random.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
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
  uint padding;       
};


struct Index {
  uint elems[3];
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(buffer_reference, std430) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference,  std430) buffer Indices {Index i[]; }; // Triangle indices
layout(set = 1, binding = 0, std430) buffer ObjDesc_ { 
    ObjDesc i[]; 
} m_objDesc;

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

vec3 compute_normal() {
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

  // Computing the normal at hit position
  vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
  // Transforming the normal to world space
  normal = normalize(vec3(normal * gl_WorldToObjectEXT));

  return normal;
}

vec3 compute_color() {
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

float traceShadows(vec3 normal, vec3 L) {  
  float attenuation = 1.0f;

  // Shadow ray
   // Tracing shadow ray only if the light is visible from the surface
  if(dot(normal, L) > 0)
  {
    float tMin   = 0.001;
    float tMax   = 1000.0;
    float bias = 1e-4f;
    vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + normal * bias;
    vec3  rayDir = L;
    uint  flags =
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed = true;
    traceRayEXT(topLevelAS,  // acceleration structure
            flags,       // rayFlags
            0xFF,        // cullMask
            0,           // sbtRecordOffset
            0,           // sbtRecordStride
            1,           // missIndex
            origin,      // ray origin
            tMin,        // ray min range
            rayDir,      // ray direction
            tMax,        // ray max range
            1            // payload (location = 1)
    );

    if(isShadowed)
    {
      attenuation = 0.3;
    }
  }

  return attenuation;
}

void main()
{
  // Compute the normal
  vec3 normal = compute_normal();

  prd.next_direction = normalize(normal + random_unit_vector(prd.seed));
  prd.next_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + normal * 1e-6f;

  // direction light
  vec3 lightDir = - normalize(pcRay.lightPosition);
  float lr = max(dot(lightDir, normal), 0.0f);
  vec3 L = vec3(lr, lr, lr) * pcRay.lightIntensity;

  // Shadow ray
  float attenuation = traceShadows(normal, L);

  if (pcRay.enableShadows == 0)
  {
    attenuation = 1.0f;
  }

  // Compute the color
  vec3 vertex_color = compute_color();
  prd.strength *= vertex_color * 0.9f * attenuation;
  
  prd.color = prd.strength;
  
}
