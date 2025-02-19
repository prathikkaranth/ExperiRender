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

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

HitPoint compute_hit_point() {
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

  // Computing the UV at hit position
  vec2 uv = vec2(v0.uv_x * barycentrics.x + v1.uv_x * barycentrics.y + v2.uv_x * barycentrics.z,
                 v0.uv_y * barycentrics.x + v1.uv_y * barycentrics.y + v2.uv_y * barycentrics.z);

  return HitPoint(normal, uv);
}

vec3 compute_diffuse(in HitPoint hit_point) {

  const MaterialRTData material = u_materials.m[gl_InstanceCustomIndexEXT];

  const vec4 diffuseSample = texture(textures[material.albedoTexIndex], hit_point.uv);
  const vec3 diffuseColor = diffuseSample.rgb * material.albedo.rgb;

  return diffuseColor;
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

  prd.next_direction = normalize(hit_point.normal + random_unit_vector(prd.seed));
  prd.next_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + hit_point.normal * 1e-6f;

  // Compute the color
  vec3 vertex_color = compute_vert_color();
 
  prd.strength *= compute_diffuse(hit_point) * vertex_color;
  prd.color = prd.strength;
  
}
