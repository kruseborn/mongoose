#include "quadtree.h"
#include "mg/mgSystem.h"
#include "rendering/rendering.h"
#include <cinttypes>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>
#include <vector>

namespace mg {

struct Quadtree {
  glm::vec4 boundary;
  size_t capacity = 4;
  std::vector<glm::vec2> points;
  Quadtree *nw, *ne, *sw, *se;
  bool isDivied;
};

void subDivide(Quadtree *qt) {
  auto b = qt->boundary;
  glm::vec4 nw = {b.x - b.z / 2, b.y + b.w / 2, b.z / 2, b.w / 2};
  glm::vec4 ne = {b.x + b.z / 2, b.y + b.w / 2, b.z / 2, b.w / 2};
  glm::vec4 sw = {b.x - b.z / 2, b.y - b.w / 2, b.z / 2, b.w / 2};
  glm::vec4 se = {b.x + b.z / 2, b.y - b.w / 2, b.z / 2, b.w / 2};
  qt->nw = new Quadtree{.boundary = nw};
  qt->ne = new Quadtree{.boundary = ne};
  qt->sw = new Quadtree{.boundary = sw};
  qt->se = new Quadtree{.boundary = se};
}

bool isInside(glm::vec4 boundary, glm::vec2 point) {
  return (point.x >= boundary.x - boundary.z && point.x <= boundary.x + boundary.z &&
          point.y >= boundary.y - boundary.w && point.y <= boundary.y + boundary.w);
}

void insert(Quadtree *qt, glm::vec2 point) {
  if (!isInside(qt->boundary, point))
    return;
  if (qt->points.size() < qt->capacity) {
    qt->points.push_back(point);
  } else {
    if (!qt->isDivied) {
      subDivide(qt);
      qt->isDivied = true;
    }
    insert(qt->nw, point);
    insert(qt->ne, point);
    insert(qt->sw, point);
    insert(qt->se, point);
  }
} // namespace mg

Quadtree qt = {.boundary = {200, 200, 200, 200}};

void simulateQuadtree() {

  for (size_t i = 0; i < 5; i++) {
    glm::vec2 point = {rand() % 400, rand() % 400};
    insert(&qt, point);
  }
}

static mg::Pipeline createSolidRendering(const mg::RenderContext &renderContext, bool points) {
  using namespace mg::shaders::solid;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;
  pipelineStateDesc.rasterization.depth.TestEnable = VK_FALSE;
  pipelineStateDesc.rasterization.rasterization.cullMode = VK_CULL_MODE_NONE;
  pipelineStateDesc.rasterization.inputAssemblyState.topology =
      points ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST: VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

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

  std::stack<Quadtree *> queue;
  queue.push(&qt);
  while (queue.size()) {
    auto q = queue.top();
    queue.pop();
    if (!q->points.size())
      continue;

    for (auto p : q->points) {
      points.push_back({p.x, p.y, 0});
    }

    lines.push_back(glm::vec3{q->boundary.x - q->boundary.z, q->boundary.y - q->boundary.z, 0});
    lines.push_back(glm::vec3{q->boundary.x + q->boundary.z, q->boundary.y - q->boundary.z, 0});

    lines.push_back(glm::vec3{q->boundary.x + q->boundary.z, q->boundary.y - q->boundary.z, 0});
    lines.push_back(glm::vec3{q->boundary.x + q->boundary.z, q->boundary.y + q->boundary.z, 0});

    lines.push_back(glm::vec3{q->boundary.x + q->boundary.z, q->boundary.y + q->boundary.z, 0});
    lines.push_back(glm::vec3{q->boundary.x - q->boundary.z, q->boundary.y + q->boundary.z, 0});

    lines.push_back(glm::vec3{q->boundary.x - q->boundary.z, q->boundary.y + q->boundary.z, 0});
    lines.push_back(glm::vec3{q->boundary.x - q->boundary.z, q->boundary.y - q->boundary.z, 0});
  }

  {
    const auto solidPipeline = createSolidRendering(renderContext, false);
    using namespace mg::shaders::solid;

    using VertexInputData = InputAssembler::VertexInputData;

    VkBuffer uniformBuffer;
    uint32_t uniformOffset;
    VkDescriptorSet uboSet;
    Ubo *ubo =
        (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
    ubo->mvp = glm::ortho(0.0f, float(vkContext.screen.width), 0.0f, float(vkContext.screen.height), -10.0f, 10.0f);

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
    vkCmdDraw(mg::vkContext.commandBuffer, lines.size(), 0, 0, 0);
  }
  {
    const auto solidPipeline = createSolidRendering(renderContext, true);
    using namespace mg::shaders::solid;

    using VertexInputData = InputAssembler::VertexInputData;

    VkBuffer uniformBuffer;
    uint32_t uniformOffset;
    VkDescriptorSet uboSet;
    Ubo *ubo =
        (Ubo *)mg::mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
    ubo->mvp = glm::ortho(0.0f, float(vkContext.screen.width), 0.0f, float(vkContext.screen.height), -10.0f, 10.0f);

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
    vkCmdDraw(mg::vkContext.commandBuffer, points.size(), 0, 0, 0);
  }
}

void renderQuadtree() {}

} // namespace mg

///// Provides an indexed free list with constant-time removals from anywhere
///// in the list without invalidating indices. T must be trivially constructible
///// and destructible.
// template <class T> class FreeList {
// public:
//  FreeList();
//
//  int insert(const T &element);
//  void erase(int n);
//  void clear();
//  // Returns the range of valid indices.
//  int range() const;
//
//  T &operator[](int n);
//  const T &operator[](int n) const;
//
// private:
//  union FreeElement {
//    T element;
//    int next;
//  };
//  std::vector<FreeElement> data;
//  int first_free;
//};
//
// template <class T> FreeList<T>::FreeList() : first_free(-1) {}
//
// template <class T> int FreeList<T>::insert(const T &element) {
//  if (first_free != -1) {
//    const int index = first_free;
//    first_free = data[first_free].next;
//    data[index].element = element;
//    return index;
//  } else {
//    FreeElement fe;
//    fe.element = element;
//    data.push_back(fe);
//    return static_cast<int>(data.size() - 1);
//  }
//}
//
// template <class T> void FreeList<T>::erase(int n) {
//  data[n].next = first_free;
//  first_free = n;
//}
//
// template <class T> void FreeList<T>::clear() {
//  data.clear();
//  first_free = -1;
//}
//
// template <class T> int FreeList<T>::range() const { return static_cast<int>(data.size()); }
//
// template <class T> T &FreeList<T>::operator[](int n) { return data[n].element; }
//
// template <class T> const T &FreeList<T>::operator[](int n) const { return data[n].element; }
//
//// Represents a node in the quadtree.
// struct QuadNode {
//  // Points to the first child if this node is a branch or the first
//  // element if this node is a leaf.
//  int32_t first_child;
//
//  // Stores the number of elements in the leaf or -1 if it this node is
//  // not a leaf.
//  int32_t count;
//};
//
//// Represents an element in the quadtree.
// struct QuadElt {
//  // Stores the ID for the element (can be used to
//  // refer to external data).
//  int id;
//
//  // Stores the rectangle for the element.
//  int x1, y1, x2, y2;
//};
//
//// Represents an element node in the quadtree.
// struct QuadEltNode {
//  // Points to the next element in the leaf node. A value of -1
//  // indicates the end of the list.
//  int next;
//
//  // Stores the element index.
//  int element;
//};
//
// struct Quadtree {
//  // Stores all the elements in the quadtree.
//  FreeList<QuadElt> elts;
//
//  // Stores all the element nodes in the quadtree.
//  FreeList<QuadEltNode> elt_nodes;
//
//  // Stores all the nodes in the quadtree. The first node in this
//  // sequence is always the root.
//  std::vector<QuadNode> nodes;
//
//  // Stores the quadtree extents.
//  QuadCRect root_rect;
//
//  // Stores the first free node in the quadtree to be reclaimed as 4
//  // contiguous nodes at once. A value of -1 indicates that the free
//  // list is empty, at which point we simply insert 4 nodes to the
//  // back of the nodes array.
//  int free_node;
//  int max_depth;
//};
//
// static QuadNodeList find_leaves(const Quadtree &tree, const QuadNodeData &root, const int rect[4]) {
//  QuadNodeList leaves, to_process;
//  to_process.push_back(root);
//  while (to_process.size() > 0) {
//    const QuadNodeData nd = to_process.pop_back();
//
//    // If this node is a leaf, insert it to the list.
//    if (tree.nodes[nd.index].count != -1)
//      leaves.push_back(nd);
//    else {
//      // Otherwise push the children that intersect the rectangle.
//      const int mx = nd.crect[0], my = nd.crect[1];
//      const int hx = nd.crect[2] >> 1, hy = nd.crect[3] >> 1;
//      const int fc = tree.nodes[nd.index].first_child;
//      const int l = mx - hx, t = my - hx, r = mx + hx, b = my + hy;
//
//      if (rect[1] <= my) {
//        if (rect[0] <= mx)
//          to_process.push_back(child_data(l, t, hx, hy, fc + 0, nd.depth + 1));
//        if (rect[2] > mx)
//          to_process.push_back(child_data(r, t, hx, hy, fc + 1, nd.depth + 1));
//      }
//      if (rect[3] > my) {
//        if (rect[0] <= mx)
//          to_process.push_back(child_data(l, b, hx, hy, fc + 2, nd.depth + 1));
//        if (rect[2] > mx)
//          to_process.push_back(child_data(r, b, hx, hy, fc + 3, nd.depth + 1));
//      }
//    }
//  }
//  return leaves;
//}

//} // namespace mg
