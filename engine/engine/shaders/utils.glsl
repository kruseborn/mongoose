#ifndef UTILS_GLSL_
#define UTILS_GLSL_

float log10(in float x) {
	return log2(x) / log2(10);
}

vec2 powv2(in vec2 b, in float e) {
	return vec2(pow(b.x, e), pow(b.y, e));
}
vec3 powv3(in vec3 b, in float e) {
	return vec3(pow(b.x, e), pow(b.y, e), pow(b.z, e));
}
vec4 powv4(in vec4 b, in float e) {
	return vec4(pow(b.x, e), pow(b.y, e), pow(b.z, e), pow(b.w, e));
}

float luminance(in vec3 rgb) {
	const vec3 kLum = vec3(0.2126, 0.7152, 0.0722);
	return max(dot(rgb, kLum), 0.0001); // prevent zero result
}

float linearizeDepth(in float depth, in mat4 projMatrix) {
	return projMatrix[3][2] / (depth - projMatrix[2][2]);
}
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

#endif // UTILS_GLSL_