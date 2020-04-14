#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform Ubo {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 cameraPosition;
} ubo;

struct Data {
  vec3 worldPosition;
  vec3 N;
	vec3 T;
  vec2 UV;
	mat3 model;
};


// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo {
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

vec4 SRGBtoLINEAR(vec4 srgbIn) {
  vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
  return vec4(linOut,srgbIn.w);
}

vec4 LINEARToSRGB(vec4 linearIn) {
  vec3 sgrbOut = pow(linearIn.xyz,vec3(1.0/2.2));
  return vec4(sgrbOut,linearIn.w);
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal(vec3 worldPosition, vec2 UV, vec3 normal, in texture2D normalTexture, in sampler linearSampler) {
    // Retrieve the tangent space matrix
    vec3 pos_dx = dFdx(worldPosition);
    vec3 pos_dy = dFdy(worldPosition);
    vec3 tex_dx = dFdx(vec3(UV, 0.0));
    vec3 tex_dy = dFdy(vec3(UV, 0.0));
    vec3 t = (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);

    vec3 ng = normalize(normal);

    t = normalize(t - ng * dot(ng, t));
    vec3 b = normalize(cross(ng, t));
    mat3 tbn = mat3(t, b, ng);

    vec3 n = texture(sampler2D(normalTexture, linearSampler), UV).rgb;
    float normalScale = 1.0;
    n = normalize(tbn * ((2.0 * n - 1.0) * vec3(normalScale, normalScale, 1.0)));
    return n;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs) {
    return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs) {
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs) {
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs) {
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}
#define linearBorder 0
#define linearRepeat 1

float reinterval(float inVal, float oldMin, float oldMax, float newMin, float newMax) {
   return (((inVal - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}

float reintervalClamp(float inVal, float oldMin, float oldMax, float newMin, float newMax) {
  return clamp(reinterval(inVal, oldMin, oldMax, newMin, newMax), newMin, newMax);
}

// Converts a normalized cartesian direction vector
// to spherical coordinates.
vec2 cartesianToSpherical(vec3 cartesian) {
  vec2 spherical;

  spherical.x = atan(cartesian.y, cartesian.x) / 3.14159;
  spherical.y = cartesian.z;

   return spherical * 0.5 + 0.5;
}

// Converts a spherical coordinate to a normalized
// cartesian direction vector.
vec3 sphericalToCartesian(vec2 spherical) {
  vec2 sinCosTheta, sinCosPhi;

  spherical = spherical * 2.0f - 1.0f;
  sinCosTheta.x = sin(spherical.x * 3.14159);
  sinCosTheta.y = cos(spherical.x * 3.14159);
  sinCosPhi = vec2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);

  return vec3(sinCosTheta.y * sinCosPhi.x, sinCosTheta.x * sinCosPhi.x, sinCosPhi.y);    
}

float linearizeDepth(float depth, float near, float far) {
  return (2.0 * near) / (far + near - depth * (far - near));
}

layout (location = 0) in Data inData;

layout(set = 1, binding = 0) uniform sampler samplers[2];
layout(set = 1, binding = 1) uniform texture2D textures[128];

layout(push_constant) uniform TextureIndices {
  int baseColorIndex;
  int normalIndex;
  int roughnessMetallicIndex;
  int emissiveIndex;
}pc;

layout (location = 0) out vec4 out_frag_color;

void main() {
  vec3 lightPos = ubo.cameraPosition.xyz;
  vec3 cameraPosition = ubo.cameraPosition.xyz;
  // Metallic and Roughness material properties are packed together
  // In glTF, these factors can be specified by fixed scalar values
  // or from a metallic-roughness map
  float perceptualRoughness = 1.0;
  float metallic = 1.0;

  // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
  // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
  vec4 orm = texture(sampler2D(textures[pc.roughnessMetallicIndex], samplers[linearBorder]), inData.UV);
  perceptualRoughness = orm.g * perceptualRoughness;
  metallic = orm.b * metallic;
  
  perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
  metallic = clamp(metallic, 0.0, 1.0);

  // Roughness is authored as perceptual roughness; as is convention,
  // convert to material roughness by squaring the perceptual roughness [2].
  float alphaRoughness = perceptualRoughness * perceptualRoughness;

  // The albedo may be defined from a base texture or a flat color
  vec4 baseColor = SRGBtoLINEAR(texture(sampler2D(textures[pc.baseColorIndex], samplers[linearBorder]), inData.UV));

  vec3 f0 = vec3(0.04);
  vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
  diffuseColor *= 1.0 - metallic;
  vec3 specularColor = mix(f0, baseColor.rgb, metallic);

  // Compute reflectance.
  float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

  // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
  // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
  float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  vec3 specularEnvironmentR0 = specularColor.rgb;
  vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

  vec3 n = getNormal(inData.worldPosition, inData.UV, inData.N, textures[pc.normalIndex], samplers[linearBorder]); // normal at surface point
  vec3 v = normalize(cameraPosition - inData.worldPosition);        // Vector from surface point to camera
  vec3 l = normalize(lightPos - inData.worldPosition);             // Vector from surface point to light
  vec3 h = normalize(l+v);                          // Half vector between both l and v
  vec3 reflection = -normalize(reflect(v, n));

  float NdotL = clamp(dot(n, l), 0.001, 1.0);
  float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
  float NdotH = clamp(dot(n, h), 0.0, 1.0);
  float LdotH = clamp(dot(l, h), 0.0, 1.0);
  float VdotH = clamp(dot(v, h), 0.0, 1.0);

  PBRInfo pbrInputs = PBRInfo(
      NdotL,
      NdotV,
      NdotH,
      LdotH,
      VdotH,
      perceptualRoughness,
      metallic,
      specularEnvironmentR0,
      specularEnvironmentR90,
      alphaRoughness,
      diffuseColor,
      specularColor
  );

  // Calculate the shading terms for the microfacet specular shading model
  vec3 F = specularReflection(pbrInputs);
  float G = geometricOcclusion(pbrInputs);
  float D = microfacetDistribution(pbrInputs);

  // Calculation of analytical lighting contribution
  vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
  vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
  // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
  vec3 lightColor = vec3(1,1,1);
  vec3 color = NdotL * lightColor * (diffuseContrib + specContrib);

  // emissive
  vec3 emissive = SRGBtoLINEAR(texture(sampler2D(textures[pc.emissiveIndex], samplers[linearBorder]), inData.UV)).rgb;
  color += emissive;

  vec4 u_ScaleFGDSpec = vec4(0,0,0,0);
  vec4 u_ScaleDiffBaseMR = vec4(0,0,0,0);

  // This section uses mix to override final color for reference app visualization
  // of various parameters in the lighting equation.
  color = mix(color, F, u_ScaleFGDSpec.x);
  color = mix(color, vec3(G), u_ScaleFGDSpec.y);
  color = mix(color, vec3(D), u_ScaleFGDSpec.z);
  color = mix(color, specContrib, u_ScaleFGDSpec.w);

  color = mix(color, diffuseContrib, u_ScaleDiffBaseMR.x);
  color = mix(color, baseColor.rgb, u_ScaleDiffBaseMR.y);
  color = mix(color, vec3(metallic), u_ScaleDiffBaseMR.z);
  color = mix(color, vec3(perceptualRoughness), u_ScaleDiffBaseMR.w);

  out_frag_color = LINEARToSRGB(vec4(color, baseColor.a));
}
