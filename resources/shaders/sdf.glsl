#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, set = 0, binding = 0) uniform Ubo {
  mat4 worldToBox;
  mat4 worldToBox2;
  vec4 info;
} ubo;

@vert

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);

    gl_Position = vec4(x, y, 0, 1);
}

@frag
#include "utils.hglsl"

layout(set = 1, binding = 0) uniform sampler samplers[2];
layout(set = 1, binding = 1) uniform texture2D textures[128];

layout (set = 2, binding = 0) uniform texture3D volumeTexture;

layout (location = 0) out vec4 outFragColor;
#define FLT_MAX 3.402823466e+38
#define FLT_MIN -3.402823466e+38

bool boxIntersect(vec3 origin, vec3 dir, out vec2 t) {
  //origin = (ubo.worldToBox * vec4(origin,1)).xyz;
  //dir = normalize((ubo.worldToBox * vec4(dir,0)).xyz);
  vec3 minBoxCorner = vec3(-1.0,-1.0,-1.0);
  vec3 maxBoxCorner = vec3(1.0,1.0,1.0);

  bool foundHit = false;
  t = vec2(FLT_MIN, FLT_MAX);
  for (int i = 0; i < 3; i++) {
    if (dir[i] == 0)
      continue;
    float invD = 1.0 / dir[i];
    float t0 = (minBoxCorner[i] - origin[i]) * invD;
    float t1 = (maxBoxCorner[i] - origin[i]) * invD;
    if (invD < 0.0) {
      float tmp = t0;
      t0 = t1;
      t1 = tmp;
    }
    foundHit = true;

    t.x = max(t.x, t0);
    t.y = min(t.y, t1);
    if(t.x >= t.y)
      return false;
  }
  
  return foundHit;
}

float map(vec3 pos) {
  pos = (ubo.worldToBox * vec4(pos,1)).xyz;
  return texture(sampler3D(volumeTexture, samplers[0]), pos).r;
}

vec3 calcNormal(vec3 p) // for function f(p)
{
    const float eps = 0.001; // or some other value
    const vec2 h = vec2(eps,0);
    return normalize(vec3(map(p+h.xyy) - map(p-h.xyy),
                           map(p+h.yxy) - map(p-h.yxy),
                           map(p+h.yyx) - map(p-h.yyx)));
}

bool isInside(vec3 pos) {
  pos = (ubo.worldToBox * vec4(pos,1)).xyz;
  return 
  pos.x >= 0 && pos.x < 1.0 &&
  pos.y >= 0 && pos.y < 1.0 &&
  pos.z >= 0 && pos.z < 1.0;
}

float castRay(vec3 ro, vec3 rd) {
  float t = 0.0;
  for(int i = 0; i < 1000; i++) {
      vec3 pos = ro + t * rd;
      if(!isInside(pos))
         return -1;
      float h = map(pos);
      if(h < 0.0001) 
          break;
      t += h;
      if(t > 20.0) break;
  }
  if(t > 20.0) t = -1.0;
  return t;
}

void main() {
  vec2 resolution = ubo.info.xy;
  vec2 mouse = ubo.info.zw;
  float an = 0;

  vec2 p = (2*vec2(gl_FragCoord)  - resolution.xy) / resolution.y;
  vec3 ro = vec3(0, 0, -3);
  vec3 ta = vec3(0, 0, 0);
  vec3 ww = normalize(ta-ro);
  vec3 uu = normalize(cross(ww, vec3(0,-1,0)));
  vec3 vv = normalize(cross(uu, ww));

  vec3 rd = normalize(vec3(p.x * uu + p.y*vv + 1.5*ww));

  vec3 col = vec3(0.4, 0.75, 1.0) - 0.7 * rd.y;
  col = mix(col, vec3(0.7, 0.75, 0.8), exp(-2.0 * rd.y));

  vec2 boxT;
  bool hit = boxIntersect(ro, rd, boxT);
  if(!hit) {
    discard;
    return;
  }
  ro = ro + rd * (boxT.x+0.00001);
  float t = castRay(ro, rd);
  vec3 pos = vec3(0);
  vec3 nor = vec3(0);
  if(t > 0.0) {
      pos = ro + t*rd;
      nor = calcNormal(pos);
      col = nor;
  }
  else 
    discard;

  col = pow(col, vec3(0.4545));
  outFragColor = vec4(nor, 1.0);
}
