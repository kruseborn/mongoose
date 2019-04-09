#include "mg/geometryUtils.h"

#include "mg/mgAssert.h"
#include "mg/mgUtils.h"
#include <glm/geometric.hpp>

namespace gm {

// Consider the line extending the segment, parameterized as v + t (w - v)
// We find projection of point p onto the line
// It falls where t = [(p-v) . (w-v)] / |w-v|^2
// We clamp t from [0,1] to handle points outside the segment vw
float distancePointToLine(const glm::vec3 &point, const mg::Line &line) {
  const auto &v = line.start;
  const auto &w = line.end;
  auto distance = glm::distance(line.start, line.end);
  if (distance == 0)
    return glm::distance(point, v); // v == w case

  auto pv = point - v;
  auto wv = w - v;
  auto t = glm::dot(pv, wv);
  t = t / (distance * distance);
  if (t < 0)
    return glm::distance(point, v);
  if (t > 1)
    return glm::distance(point, w);
  auto projection = v + wv * t;
  distance = glm::distance(point, projection);
  return distance;
}

// values = [0, 1], line edges are values close to the 0, 1
float closestEdgeFromPointToLine(const glm::vec3 &point, const mg::Line &line) {
  const auto &v = line.start;
  const auto &w = line.end;
  auto distance = glm::distance(v, w);
  if (distance == 0)
    return 0; // v == w case

  auto pv = point - v;
  auto wv = w - v;
  auto t = glm::dot(pv, wv);
  t = t / (distance * distance);
  return t;
}

float distancePointToPlane(const glm::vec3 &point, const mg::Plane &plane) {
  const auto p = glm::dot(plane.position, plane.normal);
  return std::abs(glm::dot(point, plane.normal) - p);
}

glm::vec3 projectPointOnPlane(const glm::vec3 &point, const mg::Plane &plane) {
  auto pointToPlane = point - plane.position;
  auto ndotp = glm::dot(pointToPlane, plane.normal);
  auto normalDir = plane.normal * ndotp;
  return point - normalDir;
}

bool intersectionLinePlane(glm::vec3 *out, const mg::Line &line, const mg::Plane &plane) {
  const auto u = line.end - line.start;
  const auto w = line.start - plane.position;
  const auto ndotu = glm::dot(plane.normal, u);
  if (ndotu == 0)
    return false;
  const auto ndotw = glm::dot(plane.normal, w);
  const auto si = -1.0f * ndotw / ndotu;
  if (si < 0 || si > 1) {
    return false;
  }
  *out = line.start + u * si;
  return true;
}

std::vector<bool> intersectionPlaneSpheres(const mg::Plane &plane, const std::vector<glm::vec3> &centerPositions,
                                           const std::vector<float> &sphereRadius) {
  mgAssertDesc(centerPositions.size() == sphereRadius.size(), "size error");
  std::vector<bool> res;
  res.reserve(centerPositions.size());
  auto normalLength = length(plane.normal);
  mgAssertDesc(normalLength > (1.0 - 1e-5) && normalLength < (1.0 + 1e-5), "float length error");

  auto planeDistance = glm::dot(plane.position, plane.normal);
  for (uint32_t i = 0; i < sphereRadius.size(); i++) {
    auto distance = glm::dot(centerPositions[i], plane.normal) - planeDistance;
    res.push_back(std::abs(distance) <= sphereRadius[i]);
  }
  return res;
}

mg::Line intersectionPlanePlane(const mg::Plane &plane1, const mg::Plane &plane2) {
  mg::Line res = {};
  const auto direction = glm::cross(plane1.normal, plane2.normal);
  if (length(direction) < 1e-6)
    return res;
  auto absDirection = abs(direction);
  auto n1DotP1 = glm::dot(plane1.normal, plane1.position);
  auto n2DotP2 = glm::dot(plane2.normal, plane2.position);

  glm::vec3 position;
  if (absDirection.x >= absDirection.y && absDirection.x >= absDirection.z) {
    position = glm::vec3{0.f,
                         (n2DotP2 * plane1.normal[2] - n1DotP1 * plane2.normal[2]) /
                             (plane1.normal[2] * plane2.normal[1] - plane2.normal[2] * plane1.normal[1]),
                         (n2DotP2 * plane1.normal[1] - n1DotP1 * plane2.normal[1]) /
                             (plane1.normal[1] * plane2.normal[2] - plane2.normal[1] * plane1.normal[2])};
  } else if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z) {
    position = glm::vec3{(n2DotP2 * plane1.normal[2] - n1DotP1 * plane2.normal[2]) /
                             (plane1.normal[2] * plane2.normal[0] - plane2.normal[2] * plane1.normal[0]),
                         0,
                         (n2DotP2 * plane1.normal[0] - n1DotP1 * plane2.normal[0]) /
                             (plane1.normal[0] * plane2.normal[2] - plane2.normal[0] * plane1.normal[2])};
  } else {
    position = glm::vec3{(n2DotP2 * plane1.normal[1] - n1DotP1 * plane2.normal[1]) /
                             (plane1.normal[1] * plane2.normal[0] - plane2.normal[1] * plane1.normal[0]),
                         (n2DotP2 * plane1.normal[0] - n1DotP1 * plane2.normal[0]) /
                             (plane1.normal[0] * plane2.normal[1] - plane2.normal[0] * plane1.normal[1]),
                         0};
  }
  res.start = position;
  res.end = position + direction;
  return res;
}

} // namespace gm
