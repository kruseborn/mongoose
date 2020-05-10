#include "quadtree.h"
#include "freeList.h"
#include "mg/mgSystem.h"
#include "rendering/rendering.h"
#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>
#include <vector>

namespace mg {

enum DIRS { NW, NE, SW, SE };

struct Node {
  uint32_t childId = ~0u;
  std::vector<glm::vec2> points;
};

struct Quadtree {
  size_t capacity = 8;
  std::vector<Node> nodes;
  glm::vec4 boundary;
};

void initQuadTree(Quadtree *tree, glm::vec4 boundary) {
  tree->boundary = boundary;
  tree->nodes.reserve(1000);
  tree->nodes.push_back({});
}

void subDivide(Quadtree *qt, uint32_t node, glm::vec4 b) {
  qt->nodes[node].childId = uint32_t(qt->nodes.size());
  qt->nodes.push_back({});
  qt->nodes.push_back({});
  qt->nodes.push_back({});
  qt->nodes.push_back({});
}

inline bool isInside(glm::vec2 point, glm::vec4 boundary) {
  return ((point.x >= (boundary.x - boundary.z)) & (point.x < (boundary.x + boundary.z)) &
          (point.y >= (boundary.y - boundary.w)) & (point.y < (boundary.y + boundary.w)));
}

bool _insert(Quadtree *qt, uint32_t node, glm::vec2 point, glm::vec4 b) {
  if (!isInside(point, b))
    return false;
  if (qt->nodes[node].childId == ~0u && qt->nodes[node].points.size() < qt->capacity) {
    qt->nodes[node].points.push_back(point);
    return true;
  } else {
    if (qt->nodes[node].childId == ~0u) {
      subDivide(qt, node, b);
    }
    glm::vec4 nw = {b.x - b.z / 2, b.y + b.w / 2, b.z / 2, b.w / 2};
    glm::vec4 ne = {b.x + b.z / 2, b.y + b.w / 2, b.z / 2, b.w / 2};
    glm::vec4 sw = {b.x - b.z / 2, b.y - b.w / 2, b.z / 2, b.w / 2};
    glm::vec4 se = {b.x + b.z / 2, b.y - b.w / 2, b.z / 2, b.w / 2};

    for (size_t i = 0; i < qt->nodes[node].points.size(); i++) {
      _insert(qt, qt->nodes[node].childId + NW, qt->nodes[node].points[i], nw) ||
          _insert(qt, qt->nodes[node].childId + NE, qt->nodes[node].points[i], ne) ||
          _insert(qt, qt->nodes[node].childId + SW, qt->nodes[node].points[i], sw) ||
          _insert(qt, qt->nodes[node].childId + SE, qt->nodes[node].points[i], se);
    }
    qt->nodes[node].points.clear();

    return _insert(qt, qt->nodes[node].childId + NW, point, nw) ||
           _insert(qt, qt->nodes[node].childId + NE, point, ne) ||
           _insert(qt, qt->nodes[node].childId + SW, point, sw) || _insert(qt, qt->nodes[node].childId + SE, point, se);
  }
}

void insert(Quadtree *qt, glm::vec2 point) { _insert(qt, 0, point, qt->boundary); }
Quadtree qt;

void simulateQuadtree() {

#if 0
  uint32_t totalTime = 0;
  for (int j = 0; j < 10; j++) {
    Quadtree qt;

    initQuadTree(&qt, {500, 500, 500, 500});
    auto start = mg::timer::now();

    // start time for 100000 = 210 ms, 35-45 ms release
    // release 0.314
    for (size_t i = 0; i < 100000; i++) {
      glm::vec2 point = {rand() % 1000, rand() % 1000};
      insert(&qt, point);
    }
    auto end = mg::timer::now();
    totalTime += uint32_t(mg::timer::durationInMs(start, end));
  }
  LOG("Time: " << totalTime / float(1000));

#else
  initQuadTree(&qt, {500, 500, 500, 500});

  auto start = mg::timer::now();

  for (size_t i = 0; i < 100000; i++) {
    glm::vec2 point = {rand() % 1000, rand() % 1000};
    insert(&qt, point);
  }

  LOG("Time: " << mg::timer::durationInMs(start, mg::timer::now()));
#endif
}

static mg::Pipeline createSolidRendering(const mg::RenderContext &renderContext, bool points) {
  using namespace mg::shaders::solidColor;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.inputAssemblyState.topology =
      points ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  const auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

void renderQuadTree(const mg::RenderContext &renderContext) {

  std::vector<glm::vec3> lines;
  std::vector<glm::vec3> points;
  struct N {
    Node *node;
    glm::vec4 b;
  };

  std::stack<N> queue;
  queue.push({&qt.nodes.front(), qt.boundary});
  while (queue.size()) {
    auto n = queue.top();
    queue.pop();

    if (n.node->childId != -1) {
      glm::vec4 nw = {n.b.x - n.b.z / 2, n.b.y + n.b.w / 2, n.b.z / 2, n.b.w / 2};
      glm::vec4 ne = {n.b.x + n.b.z / 2, n.b.y + n.b.w / 2, n.b.z / 2, n.b.w / 2};
      glm::vec4 sw = {n.b.x - n.b.z / 2, n.b.y - n.b.w / 2, n.b.z / 2, n.b.w / 2};
      glm::vec4 se = {n.b.x + n.b.z / 2, n.b.y - n.b.w / 2, n.b.z / 2, n.b.w / 2};

      queue.push({&qt.nodes[n.node->childId + NW], nw});
      queue.push({&qt.nodes[n.node->childId + NE], ne});
      queue.push({&qt.nodes[n.node->childId + SW], sw});
      queue.push({&qt.nodes[n.node->childId + SE], se});
    }

    for (auto p : n.node->points) {
      points.push_back({p.x, p.y, 0});
    }

    lines.push_back(glm::vec3{n.b.x - n.b.z, n.b.y - n.b.w, 0});
    lines.push_back(glm::vec3{n.b.x + n.b.z, n.b.y - n.b.w, 0});

    lines.push_back(glm::vec3{n.b.x + n.b.z, n.b.y - n.b.w, 0});
    lines.push_back(glm::vec3{n.b.x + n.b.z, n.b.y + n.b.w, 0});

    lines.push_back(glm::vec3{n.b.x + n.b.z, n.b.y + n.b.w, 0});
    lines.push_back(glm::vec3{n.b.x - n.b.z, n.b.y + n.b.w, 0});

    lines.push_back(glm::vec3{n.b.x - n.b.z, n.b.y + n.b.w, 0});
    lines.push_back(glm::vec3{n.b.x - n.b.z, n.b.y - n.b.w, 0});
  }
  {
    const auto solidPipeline = createSolidRendering(renderContext, false);
    using namespace mg::shaders::solidColor;

    using VertexInputData = InputAssembler::VertexInputData;

    VkBuffer uniformBuffer;
    uint32_t uniformOffset;
    VkDescriptorSet uboSet;
    Ubo *ubo =
        (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
    ubo->mvp = glm::ortho(0.0f, float(vkContext.screen.width), 0.0f, float(vkContext.screen.height), -10.0f, 10.0f);
    static glm::vec4 colors[] = {{1, 0, 0, 1}, {0, 1, 0, 1},        {0, 0, 1, 1},
                                 {1, 1, 1, 1}, {1.5f, 0.5, 0.5, 1}, {0.5f, 0.5, 1, 1}};
    auto color = glm::vec4{1, 0, 0, 1};
    ubo->color = color;

    VkBuffer vertexBuffer;
    VkDeviceSize vertexBufferOffset;
    VertexInputData *vertices = (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
        mg::sizeofContainerInBytes(lines), &vertexBuffer, &vertexBufferOffset);

    memcpy(vertices, lines.data(), mg::sizeofContainerInBytes(lines));

    DescriptorSets descriptorSets = {};
    descriptorSets.ubo = uboSet;

    uint32_t offsets[] = {uniformOffset, 0};
    vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline.layout, 0,
                            mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(offsets), offsets);

    vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline.pipeline);

    vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    vkCmdDraw(mg::vkContext.commandBuffer, uint32_t(lines.size()), 1, 0, 0);
  }
  {
    const auto solidPipeline = createSolidRendering(renderContext, true);
    using namespace mg::shaders::solidColor;

    using VertexInputData = InputAssembler::VertexInputData;

    VkBuffer uniformBuffer;
    uint32_t uniformOffset;
    VkDescriptorSet uboSet;
    Ubo *ubo =
        (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
    ubo->mvp = glm::ortho(0.0f, float(vkContext.screen.width), 0.0f, float(vkContext.screen.height), -10.0f, 10.0f);
    ubo->color = glm::vec4(0, 1, 0, 1);

    VkBuffer vertexBuffer;
    VkDeviceSize vertexBufferOffset;
    VertexInputData *vertices = (VertexInputData *)mg::mgSystem.linearHeapAllocator.allocateBuffer(
        mg::sizeofContainerInBytes(points), &vertexBuffer, &vertexBufferOffset);

    memcpy(vertices, points.data(), mg::sizeofContainerInBytes(points));

    DescriptorSets descriptorSets = {};
    descriptorSets.ubo = uboSet;

    uint32_t offsets[] = {uniformOffset, 0};
    vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline.layout, 0,
                            mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(offsets), offsets);

    vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline.pipeline);

    vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    vkCmdDraw(mg::vkContext.commandBuffer, uint32_t(points.size()), 1, 0, 0);
  }
} 

} // namespace mg

