#pragma once

class v8f {
public:
  __m256 __declspec(align(32)) v;

  v8f() { v = _mm256_setzero_ps(); }
  v8f(float f) { v = _mm256_set1_ps(f); }
  v8f(float const *p) { v = _mm256_load_ps(p); }
  v8f(__m256 const &x) { v = x; }

  void store(float *p) { _mm256_store_ps(p, v); }
};

inline v8f operator*(const v8f &a, const v8f &b) { return _mm256_mul_ps(a.v, b.v); }
inline v8f operator+(const v8f &a, const v8f &b) { return _mm256_add_ps(a.v, b.v); }
inline v8f operator-(const v8f &a, const v8f &b) { return _mm256_sub_ps(a.v, b.v); }
inline v8f operator/(const v8f &a, const v8f &b) { return _mm256_div_ps(a.v, b.v); }
inline v8f operator>(const v8f &a, const v8f &b) { return _mm256_cmp_ps(b.v, a.v, 1); }
inline v8f operator<(const v8f &a, const v8f &b) { return _mm256_cmp_ps(a.v, b.v, 1); }
inline v8f &operator+=(v8f &a, v8f const &b) {
  a = a + b;
  return a;
}

inline v8f sqrt(const v8f &a) { return _mm256_sqrt_ps(a.v); }
inline v8f if_select(v8f const &s, const v8f &a, const v8f &b) { return _mm256_blendv_ps(b.v, a.v, s.v); }
inline v8f length(const v8f &a, const v8f &b) {
  v8f length = sqrt(a * a + b * b);
  return length;
}