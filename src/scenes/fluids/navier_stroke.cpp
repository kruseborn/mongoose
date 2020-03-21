#include "navier_stroke.h"
#include "mg/window.h"
#include "rendering/rendering.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// incrompressible fluid: water
// compressible fluid: air

namespace mg {

FluidTimes fluidTimes = {};

static constexpr uint32_t size = (256 + 2) * (256 + 2);
static constexpr float force = 5.0f;
static constexpr float source = 100.0f;
static constexpr float visc = 0.0f;
static constexpr float diff = 0.0f;

static float u[size] = {}, v[size] = {}, u_prev[size] = {}, v_prev[size] = {};
static float dens[size] = {}, dens_prev[size] = {};
static float s[size] = {};

#define IX(i, j) ((i) + (N + 2) * (j))

void set_bnd(int N, int b, float *x) {
  int i;
  for (i = 1; i <= N; i++) {
    x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
    x[IX(N + 1, i)] = b == 1 ? -x[IX(N, i)] : x[IX(N, i)];
    x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
    x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
  }
  x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
  x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
  x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
  x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

void diffuse(int N, int b, float *x, float *x0, float diff, float dt) {
  float a = dt * diff * N * N;
  for (size_t iterations = 0; iterations < 5; iterations++) {
    for (size_t j = 0; j < N; j++) {
      for (size_t i = 0; i < N; i++) {
        x[IX(i, j)] = (x0[IX(i, j)] + a * (x[IX(i - 1, j)] + x[IX(i + 1, j)] +
                                           x[IX(i, j - 1)] + x[IX(i, j + 1)])) /
                      (1 + 4 * a);
      }
    }
  }
  set_bnd(N, b, x);
} // namespace mg

void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt) {
  int i, j, i0, j0, i1, j1;
  float s0, t0, s1, t1, dt0;
  dt0 = dt * N;
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      float prevX = i - dt0 * u[IX(i, j)];
      float prevY = j - dt0 * v[IX(i, j)];
      prevX = std::max(prevX, 0.5f);
      prevX = std::min(prevX, N + 0.5f);
      prevY = std::max(prevY, 0.5f);
      prevY = std::min(prevY, N + 0.5f);
      i0 = (int)prevX;
      i1 = i0 + 1;
      j0 = (int)prevY;
      j1 = j0 + 1;

      s1 = prevX - i0;
      s0 = 1 - s1;
      t1 = prevY - j0;
      t0 = 1 - t1;
      d[IX(i, j)] = s0 * (t0 * d0[IX(i0, j0)] + t1 * d0[IX(i0, j1)]) +
                    s1 * (t0 * d0[IX(i1, j0)] + t1 * d0[IX(i1, j1)]);
    }
  }
  set_bnd(N, b, d);
}

void project(int N, float *u, float *v, float *p, float *div) {
  int i, j, k;
  float h;
  h = 1.0f / N;
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      div[IX(i, j)] =
          -0.5f * h *
          (u[IX(i + 1, j)] - u[IX(i - 1, j)] + v[IX(i, j + 1)] - v[IX(i, j - 1)]);
      p[IX(i, j)] = 0;
    }
  }
  set_bnd(N, 0, div);
  set_bnd(N, 0, p);
  for (k = 0; k < 20; k++) {
    for (i = 1; i <= N; i++) {
      for (j = 1; j <= N; j++) {
        p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] +
                       p[IX(i, j - 1)] + p[IX(i, j + 1)]) /
                      4;
      }
    }
    set_bnd(N, 0, p);
  }
  for (i = 1; i <= N; i++) {
    for (j = 1; j <= N; j++) {
      u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
      v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
    }
  }
  set_bnd(N, 1, u);
  set_bnd(N, 2, v);
}

void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt) {
  {
    auto start = mg::timer::now();
    diffuse(N, 1, u0, u, visc, dt);
    diffuse(N, 2, v0, v, visc, dt);
    auto end = mg::timer::now();
    fluidTimes.diffuseSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();
    project(N, u0, v0, u, v);
    auto end = mg::timer::now();
    fluidTimes.projectSum += uint32_t(mg::timer::durationInMs(start, end));
  }

  {
    auto start = mg::timer::now();
    advect(N, 1, u, u0, u0, v0, dt);
    advect(N, 2, v, v0, u0, v0, dt);
    auto end = mg::timer::now();
    fluidTimes.advecSum += uint32_t(mg::timer::durationInMs(start, end));
  }
  
  project(N, u, v, u0, v0);

  // density step
  diffuse(N, 0, s, dens, diff, dt);
  advect(N, 0, dens, s, u, v, dt);
}

static void get_from_UI(int N, float *d, float *u, float *v,
                        const mg::FrameData &frameData) {

  if (frameData.mouse.left) {
    int i, j;
    i = int(frameData.mouse.xy.x * N);
    j = int((1.0f - frameData.mouse.xy.y) * N);
    auto delta = frameData.mouse.xy - frameData.mouse.prevXY;

    for (int y = j - 4; y <= j + 4; y++) {
      for (int x = i - 4; x <= i + 4; x++) {
        u[IX(x, y)] += 50.0f * delta.x;
        v[IX(x, y)] += 50.0f * (-1.0f * delta.y);
        d[IX(x, y)] += 0.5;
      }
    }
  }
}

void simulateNavierStroke(const mg::FrameData &frameData) {
  float dt = 0.1f;
  uint32_t N = 256;
  get_from_UI(N, dens, u, v, frameData);
  vel_step(N, u, v, u_prev, v_prev, visc, dt);
  for (size_t i = 0; i < size; i++) {
    dens[i] *= 0.95f;
  }
}

void renderNavierStroke(const mg::RenderContext &renderContext) {
  renderFluid(renderContext, dens, size);
}

} // namespace mg