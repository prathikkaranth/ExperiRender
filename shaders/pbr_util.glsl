const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
// Enhanced Fresnel with roughness for improved energy conservation
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
// Alternative microfacet distributions
// Beckmann distribution (alternative to GGX)
float DistributionBeckmann(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float NdotH4 = NdotH2 * NdotH2;
    
    float exponent = (NdotH2 - 1.0) / (a2 * NdotH2);
    return exp(exponent) / (PI * a2 * NdotH4);
}
// ----------------------------------------------------------------------------
// Anisotropic GGX distribution for surfaces with directional roughness
float DistributionGGXAnisotropic(vec3 N, vec3 H, vec3 T, vec3 B, float roughnessX, float roughnessY)
{
    float ax = roughnessX * roughnessX;
    float ay = roughnessY * roughnessY;
    
    float HdotT = dot(H, T);
    float HdotB = dot(H, B);
    float NdotH = dot(N, H);
    
    float denomSqr = (HdotT * HdotT) / (ax * ax) + (HdotB * HdotB) / (ay * ay) + NdotH * NdotH;
    return 1.0 / (PI * ax * ay * denomSqr * denomSqr);
}
// ----------------------------------------------------------------------------
// Enhanced geometry function with height correlation
float GeometrySmithCorrelated(vec3 N, vec3 V, vec3 L, float roughness)
{
    float a = roughness * roughness;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    float lambdaV = (-1.0 + sqrt(1.0 + a * a * (1.0 - NdotV * NdotV) / (NdotV * NdotV))) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + a * a * (1.0 - NdotL * NdotL) / (NdotL * NdotL))) * 0.5;
    
    return 1.0 / (1.0 + lambdaV + lambdaL);
}
// ----------------------------------------------------------------------------
// Oren-Nayar diffuse BRDF for rough diffuse surfaces
vec3 OrenNayarDiffuse(vec3 L, vec3 V, vec3 N, vec3 albedo, float roughness)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    
    float roughness2 = roughness * roughness;
    
    float A = 1.0 - 0.5 * (roughness2 / (roughness2 + 0.57));
    float B = 0.45 * (roughness2 / (roughness2 + 0.09));
    
    vec3 L_proj = normalize(L - N * NdotL);
    vec3 V_proj = normalize(V - N * NdotV);
    
    float cos_phi_diff = max(0.0, dot(L_proj, V_proj));
    
    float sin_alpha = sqrt(1.0 - NdotL * NdotL);
    float sin_beta = sqrt(1.0 - NdotV * NdotV);
    
    float tan_alpha = sin_alpha / max(NdotL, 1e-8);
    float tan_beta = sin_beta / max(NdotV, 1e-8);
    
    return albedo / PI * (A + B * cos_phi_diff * sin_alpha * tan_beta);
}