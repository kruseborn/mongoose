#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, set = 0, binding = 0) uniform UBO {
  mat4 boxToWorld;
  mat4 worldToBox;
  mat4 mv;
  vec4 cameraPosition;
  vec4 color;
  vec4 minMaxIsoValue;
  vec4 nrOfVoxels;
} ubo;

@vert
layout (location = 0) out vec2 outUV;

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    outUV.x = (x+1.0)*0.5;
    outUV.y = (y+1.0)*0.5;

    gl_Position = vec4(x, y, 0, 1);
}

@frag

#include "utils.hglsl"
#include "pbr.hglsl"

layout (set = 1, binding = 0) uniform sampler2D samplerFront;
layout (set = 2, binding = 0) uniform sampler2D samplerBack;
layout (set = 3, binding = 0) uniform sampler3D samplerVolume;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

const float uint16MaxValue = (1<<16)-1; //(2^16-1)

float normalizeVoxelValue(float value) {
  value = value * uint16MaxValue;
  return reinterval(value, ubo.minMaxIsoValue.x, ubo.minMaxIsoValue.y, 0, 1);
}

vec3 computeGradient(sampler3D volume, vec3 texcoord3d, vec3 delta) {
  vec3 gradient = vec3(0.0);
  gradient.x = (normalizeVoxelValue(texture(volume, texcoord3d + vec3(-delta.x, 0.0, 0.0)).r) -
                normalizeVoxelValue(texture(volume, texcoord3d + vec3(delta.x, 0.0, 0.0)).r));
  gradient.y = (normalizeVoxelValue(texture(volume, texcoord3d + vec3(0.0, -delta.y, 0.0)).r) -
                normalizeVoxelValue(texture(volume, texcoord3d + vec3(0.0, delta.y, 0.0)).r));
  gradient.z = (normalizeVoxelValue(texture(volume, texcoord3d + vec3(0.0, 0.0, -delta.z)).r) -
                normalizeVoxelValue(texture(volume, texcoord3d + vec3(0.0, 0.0, delta.z)).r));
  return gradient;
}

struct World {
  vec4 sky;
  vec4 ground;
  vec3 up;
};

struct Ray {
  vec3 rayDir;
  float rayLength;
};

struct RayHit {
  vec3 position;
  float isoValue;
  bool hit;
};

Ray getRay(vec3 start, vec3 stop) {
  vec3 rayDir = stop - start;
  float rayLength = length(rayDir);
  rayDir = normalize(rayDir);
  return Ray(rayDir, rayLength);
}

vec4 diffuseEnvLight(vec3 normal, World world) {
  float ndotu = dot(normal, world.up);
  return mix(world.ground, world.sky, 0.5 * ndotu + 0.5);
}

bool isInside(vec3 position) {
  return 
  position.x >= 0 && position.x < 1.0 &&
  position.y >= 0 && position.y < 1.0 &&
  position.z >= 0 && position.z < 1.0;
}

RayHit castRay(vec3 startPosition, Ray ray, float stepSize, float threshold, int start) {
  float numOfSteps = ray.rayLength / stepSize;

  bool hit = false;
  float isoValue = 0;
  vec3 position = vec3(0,0,0);
  for(int i = start; i < numOfSteps; i++) {
    position = startPosition + ray.rayDir * float(i) * stepSize;
    if(!isInside(position))
      continue;
    isoValue = normalizeVoxelValue(texture(samplerVolume, position).r);
    if(isoValue >= threshold) {
      hit = true;
      break;
    }
  }
  return RayHit(position, isoValue, hit);
}

vec3 binarySearch(vec3 position, Ray ray, float stepSize, float threshold) {
  vec3 left = position - ray.rayDir * stepSize;
  vec3 right = position;

  for(int i = 0; i < 5; i++) {
    vec3 middle = (left + right) / 2;
    float isoValue = normalizeVoxelValue(texture(samplerVolume, middle).r);
    if(isoValue > threshold)
      right = middle;
    else
      left = middle;
  }
  return (right + left) / 2;
}

float castShadowRay(vec3 position, vec3 L, float stepSize, float threshold) {
  vec3 boxL = normalize(inverse(transpose(mat3(ubo.worldToBox))) * L);
  Ray rayToLight = Ray(boxL, stepSize * 500.0);
  RayHit rayLightHit = castRay(position, rayToLight, stepSize, threshold, 5);
  return rayLightHit.hit ? 0.3 : 1.0;
}

// Isosurface rendering
vec3 isoSurface(vec3 rayStart, vec3 rayStop, float stepSize, float threshold) {
  // cast ray
  Ray ray = getRay(rayStart, rayStop);
  RayHit rayhit = castRay(rayStart, ray, stepSize, threshold, 0);
  if(!rayhit.hit)
    return vec3(0);

  vec3 position = binarySearch(rayhit.position, ray, stepSize, threshold);
  vec3 lightPos = ubo.cameraPosition.xyz;

  // set color  
  vec3 delta = 1.0 / textureSize(samplerVolume, 0);
  vec3 gradient = computeGradient(samplerVolume, position, delta);

  mat4 toWorldSpace = ubo.boxToWorld;
  vec3 N =  normalize(transpose(inverse(mat3(toWorldSpace))) * gradient);
  vec3 P = (toWorldSpace * vec4(position, 1.0)).xyz;
  vec3 V = normalize(ubo.cameraPosition.xyz - P); 
  vec3 L = normalize(lightPos - P);
  vec3 H = normalize(L+V);
  vec3 reflection = -normalize(reflect(V, N));

  float shadow = castShadowRay(position, L, stepSize, threshold);

  float NdotL = clamp(dot(N, L), 0.001, 1.0);
  float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
  float NdotH = clamp(dot(N, H), 0.0, 1.0);
  float LdotH = clamp(dot(L, H), 0.0, 1.0);
  float VdotH = clamp(dot(V, H), 0.0, 1.0);

  float perceptualRoughness = 0.3;
  float metallic = 0.1;
  float alphaRoughness = perceptualRoughness * perceptualRoughness;

  vec3 baseColor = vec3(0.1, 0.6, 0.1);
  vec3 f0 = vec3(0.04);
  vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
  diffuseColor *= 1.0 - metallic;
  vec3 specularColor = mix(f0, baseColor.rgb, metallic);

  float reflectance = 1.0;
  float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  vec3 specularEnvironmentR0 = specularColor;
  vec3 specularEnvironmentR90 =  vec3(1.0) * reflectance90;

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
  vec3 F = specularReflection(pbrInputs);
  float G = geometricOcclusion(pbrInputs);
  float D = microfacetDistribution(pbrInputs);

  vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
  vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);

  World world;
  world.sky = vec4(135.0/255.0, 206.0/255.0, 250.0/255.0, 1.0);
  world.ground = vec4(87.0/255.0, 59.0/255.0, 12.0/255.0, 1.0);
  world.up = vec3(0,0,-1);

  vec3 lightColor = diffuseEnvLight(N, world).xyz;
  vec3 finalColor = NdotL * lightColor * (diffuseContrib + specContrib) * shadow;
  return finalColor;
}

void main() {
  vec2 textCoord = inUV;
  textCoord.y = 1.0 - textCoord.y;
  vec3 rayStart = texture(samplerFront, textCoord).xyz;
  vec3 rayStop = texture(samplerBack, textCoord).xyz;

  vec3 color = isoSurface(rayStart, rayStop, 0.001, normalizeVoxelValue(ubo.minMaxIsoValue.z/ uint16MaxValue));
  color = pow(color, vec3(0.45));
  outFragColor = vec4(color, 1.0);
}
