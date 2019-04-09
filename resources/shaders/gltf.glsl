#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBO {
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

@vert
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_tangent;
layout (location = 3) in vec2 in_texCoord;
layout (location = 0) out Data outData;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
  outData.worldPosition = (ubo.model * vec4(in_position, 1.0)).xyz;
	outData.N = mat3(ubo.model) * in_normal;
	outData.T = mat3(ubo.model) * in_tangent.xyz;
	outData.UV = in_texCoord;

	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
}

@frag

#include "pbr.hglsl"

layout (location = 0) in Data inData;
layout (set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout (set = 2, binding = 0) uniform sampler2D normalTexture;
layout (set = 3, binding = 0) uniform sampler2D roughnessMetallicTexture;
layout (set = 4, binding = 0) uniform sampler2D emissiveTexture;

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
  vec4 orm = texture(roughnessMetallicTexture, inData.UV);
  perceptualRoughness = orm.g * perceptualRoughness;
  metallic = orm.b * metallic;
  
  perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
  metallic = clamp(metallic, 0.0, 1.0);

  // Roughness is authored as perceptual roughness; as is convention,
  // convert to material roughness by squaring the perceptual roughness [2].
  float alphaRoughness = perceptualRoughness * perceptualRoughness;

  // The albedo may be defined from a base texture or a flat color
  vec4 baseColor = SRGBtoLINEAR(texture(baseColorTexture, inData.UV));

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

  vec3 n = getNormal(inData.worldPosition, inData.UV, inData.N, normalTexture);                             // normal at surface point
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
  vec3 emissive = SRGBtoLINEAR(texture(emissiveTexture, inData.UV)).rgb;
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