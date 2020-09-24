#version 450 core
#extension GL_ARB_shading_language_420pack : enable

//RAY MARCH METHOD IS COPIED FROM https://code.google.com/archive/p/efficient-sparse-voxel-octrees/
#define STACK_SIZE 23 //must be 23
#define eps 3.552713678800501e-15

#define FLT_MAX 3.402823466e+38
#define FLT_MIN -3.402823466e+38

layout (std140, set = 0, binding = 0) uniform Ubo {
	mat4 uProjection;
	mat4 uView;
	vec4 uPosition;
	ivec4 screenSize;
} ubo;

@vert

void main() {
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);

    gl_Position = vec4(x, y, 0, 1);
}

@frag
layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samplers[2];
layout(set = 1, binding = 1) uniform texture2D textures[128];
layout (set = 2, binding = 0) uniform texture3D volumeTexture;

layout(set = 3, binding = 0) buffer uuOctree { 
  uint values[]; 
} uOctree;

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

uint iter = 0;
struct StackItem { uint node; float t_max; } stack[STACK_SIZE];
bool RayMarchLeaf(vec3 o, vec3 d, out float o_t, out float o_t_max, out vec3 o_color, out vec3 o_normal) {
	d.x = abs(d.x) > eps ? d.x : (d.x >= 0 ? eps : -eps);
	d.y = abs(d.y) > eps ? d.y : (d.y >= 0 ? eps : -eps);
	d.z = abs(d.z) > eps ? d.z : (d.z >= 0 ? eps : -eps);

	// Precompute the coefficients of tx(x), ty(y), and tz(z).
	// The octree is assumed to reside at coordinates [1, 2].
	vec3 t_coef = 1.0f / -abs(d);
	vec3 t_bias = t_coef * o;

	uint oct_mask = 0u;
	if(d.x > 0.0f) oct_mask ^= 1u, t_bias.x = 3.0f * t_coef.x - t_bias.x;
	if(d.y > 0.0f) oct_mask ^= 2u, t_bias.y = 3.0f * t_coef.y - t_bias.y;
	if(d.z > 0.0f) oct_mask ^= 4u, t_bias.z = 3.0f * t_coef.z - t_bias.z;

	// Initialize the active span of t-values.

	float t_min = max(max(2.0f * t_coef.x - t_bias.x, 2.0f * t_coef.y - t_bias.y), 2.0f * t_coef.z - t_bias.z);
	float t_max = min(min(       t_coef.x - t_bias.x,        t_coef.y - t_bias.y),        t_coef.z - t_bias.z);
	t_min = max(t_min, 0.0f);
	float h = t_max;

	float prev_t_min = t_min;

	uint parent = 0u;
	uint cur    = 0u;
	vec3 pos    = vec3(1.0f);
	uint idx    = 0u;
	if(1.5f * t_coef.x - t_bias.x > t_min) idx ^= 1u, pos.x = 1.5f;
	if(1.5f * t_coef.y - t_bias.y > t_min) idx ^= 2u, pos.y = 1.5f;
	if(1.5f * t_coef.z - t_bias.z > t_min) idx ^= 4u, pos.z = 1.5f;

	uint  scale      = STACK_SIZE - 1;
	float scale_exp2 = 0.5f; //exp2( scale - STACK_SIZE )

	while( scale < STACK_SIZE ) {
		++iter;
		if(cur == 0u) cur = uOctree.values[ parent + ( idx ^ oct_mask ) ];
		// Determine maximum t-value of the cube by evaluating
		// tx(), ty(), and tz() at its corner.

		vec3 t_corner = pos * t_coef - t_bias;
		float tc_max = min(min(t_corner.x, t_corner.y), t_corner.z);

		if( (cur & 0x80000000u) != 0 && t_min <= t_max ) {
			float tv_max = min(t_max, tc_max);
			float half_scale_exp2 = scale_exp2 * 0.5f;
			vec3 t_center = half_scale_exp2 * t_coef + t_corner;

			if( t_min <= tv_max ) {
				if( (cur & 0x40000000u) != 0 ) // leaf node
					break;

				// PUSH
				if( tc_max < h ) {
					stack[ scale ].node = parent;
					stack[ scale ].t_max = t_max;
				}
				h = tc_max;

				parent = cur & 0x3fffffffu;

				idx = 0u;
				--scale;
				scale_exp2 = half_scale_exp2;
				if(t_center.x > t_min) idx ^= 1u, pos.x += scale_exp2;
				if(t_center.y > t_min) idx ^= 2u, pos.y += scale_exp2;
				if(t_center.z > t_min) idx ^= 4u, pos.z += scale_exp2;

				cur = 0;
				t_max = tv_max;

				continue;
			}
		}

		//ADVANCE
		uint step_mask = 0u;
		if(t_corner.x <= tc_max) step_mask ^= 1u, pos.x -= scale_exp2;
		if(t_corner.y <= tc_max) step_mask ^= 2u, pos.y -= scale_exp2;
		if(t_corner.z <= tc_max) step_mask ^= 4u, pos.z -= scale_exp2;

		// Update active t-span and flip bits of the child slot index.
		prev_t_min = t_min;
		t_min = tc_max;
		idx ^= step_mask;

		// Proceed with pop if the bit flips disagree with the ray direction.
		if( (idx & step_mask) != 0 ) {
			// POP
			// Find the highest differing bit between the two positions.
			uint differing_bits = 0;
			if ((step_mask & 1u) != 0) differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp2);
			if ((step_mask & 2u) != 0) differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp2);
			if ((step_mask & 4u) != 0) differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp2);
			scale = findMSB(differing_bits);
			scale_exp2 = uintBitsToFloat((scale - STACK_SIZE + 127u) << 23u); // exp2f(scale - s_max)

			// Restore parent voxel from the stack.
			parent = stack[scale].node;
			t_max  = stack[scale].t_max;

			// Round cube position and extract child slot index.
			uint shx = floatBitsToUint(pos.x) >> scale;
			uint shy = floatBitsToUint(pos.y) >> scale;
			uint shz = floatBitsToUint(pos.z) >> scale;
			pos.x = uintBitsToFloat(shx << scale);
			pos.y = uintBitsToFloat(shy << scale);
			pos.z = uintBitsToFloat(shz << scale);
			idx  = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

			// Prevent same parent from being stored again and invalidate cached child descriptor.
			h = 0.0f;
			cur = 0;
		}
	}

	o_t = t_min;
	o_t_max = t_max;

	return scale < STACK_SIZE && t_min <= t_max;
}

float map(vec3 pos) {
	pos = pos-1;
  return texture(sampler3D(volumeTexture, samplers[0]), pos).r;
}
vec3 calcNormal(vec3 p) // for function f(p)
{
    const float _eps = 0.1; // or some other value
    const vec2 h = vec2(_eps,0);
    return normalize(vec3(map(p+h.xyy) - map(p-h.xyy),
                           map(p+h.yxy) - map(p-h.yxy),
                           map(p+h.yyx) - map(p-h.yyx)));
}

vec3 norm(float t) {
	vec2 p = (2*vec2(gl_FragCoord)  - ubo.screenSize.xy) / ubo.screenSize.y;

	vec3 ro = vec3(0, 0, -3);
  vec3 ta = vec3(0, 0, 0);
  vec3 ww = normalize(ta-ro);
  vec3 uu = normalize(cross(ww, vec3(0,-1,0)));
  vec3 vv = normalize(cross(uu, ww));

  vec3 rd = normalize(vec3(p.x * uu + p.y*vv + 1.5*ww));

	vec3 pos = ro + rd * t;
	pos = (pos + 1) * 0.5;
	vec3 n = calcNormal(pos);
	return n;
}

bool isInside(vec3 pos) {
	pos = pos-1;
  return 
  pos.x >= 0 && pos.x < 1.0 &&
  pos.y >= 0 && pos.y < 1.0 &&
  pos.z >= 0 && pos.z < 1.0;
}

void swap(out float a, out float b) {
	float tmp = a;
	a = b;
	b = tmp;
}

vec3 calcPosition(float t_min, float t_max, vec3 ro, vec3 rd) {
	float t = 0.0;
	vec3 pos;
  for(int i = 0; i < 1000; i++) {
      pos = ro + t * rd;
			if(!isInside(pos))
				return vec3(0,0,1);
      float h = map(pos);
      if(h < 0.000001) 
          break;
      t += h;
      if(t > 20) return vec3(0,0,0);
  }
  if(t > 20) return vec3(0,0,0);
  return pos;
}

void main() {
	vec2 p = (2*vec2(gl_FragCoord)  - ubo.screenSize.xy) / ubo.screenSize.y;
  vec3 ro = vec3(0,0,-3);
  vec3 ta = vec3(0,0,0);
  vec3 ww = normalize(ta-ro);
  vec3 uu = normalize(cross(ww, vec3(0,-1,0)));
  vec3 vv = normalize(cross(uu, ww));

  vec3 rd = normalize(vec3(p.x * uu + p.y*vv + 1.5*ww));

	vec2 boxT;
  bool hit2 = boxIntersect(ro, rd, boxT);
  if(!hit2) {
 		outFragColor = vec4(vec3(0.0,0.0,0.0), 1);
    return;
  }
	ro = ro + rd * (boxT.x+0.00001);
	vec3 roOctree = (ro * 0.5) + 1.5;

	float t_min, t_max; vec3 color, normal;
	bool hit = RayMarchLeaf(roOctree, rd, t_min, t_max, color, normal);
	if(!hit) {
		outFragColor = vec4(vec3(0,0,0), 1);
		return;
	}
	t_min = t_min - 0.00000001;
	t_max = t_max + 0.00000001;
	roOctree = roOctree + t_min * rd;
	vec3 pos = calcPosition(t_min, t_max, roOctree, rd);
	outFragColor = vec4(pos-1, 1);
}
