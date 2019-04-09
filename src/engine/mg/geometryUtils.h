#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace mg {

struct Triangle {
  glm::vec3 position1, position2, position3;
};
struct Plane {
  glm::vec3 position;
  glm::vec3 normal;
};
struct Line {
  glm::vec3 start, end;
};

float distancePointToLine(const glm::vec3 &point, const mg::Line &line);
float closestEdgeFromPointToLine(const glm::vec3 &point, const mg::Line &line);
float distancePointToPlane(const glm::vec3 &point, const mg::Plane &plane);

glm::vec3 projectPointOnPlane(const glm::vec3 &point, const mg::Plane &plane);

bool intersectionLinePlane(glm::vec3 *out, const Line &line, const Plane &plane);
std::vector<bool> intersectionPlaneSpheres(const Plane &plane, const std::vector<glm::vec3> &centerPositions,
                                           const std::vector<float> &sphereRadius);

Line intersectionPlanePlane(const Plane &plane1, const Plane &plane2);

} // namespace mg
