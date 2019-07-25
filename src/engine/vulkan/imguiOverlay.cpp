#include "imguiOverlay.h"
#include "vkUtils.h"
#include "mg/textureContainer.h"
#include "pipelineContainer.h"
#include "rendering/rendering.h"

#include "mg/mgSystem.h"
#include "mg/window.h"
#include <imgui.h>
#include "mg/tools.h"

namespace mg {

struct ImguiBuffer {
  VkBuffer buffer;
  VkDeviceSize bufferOffset;
  VkDeviceSize indicesOffset;
};

ImguiBuffer allocateImguiBuffer() {
  ImDrawData* imDrawData = ImGui::GetDrawData();
  VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  VkBuffer buffer;
  VkDeviceSize bufferOffset;
  auto hostVisibleMemory = (char*)mg::mgSystem.linearHeapAllocator.allocateBuffer(vertexBufferSize + indexBufferSize, &buffer, &bufferOffset);

  uint32_t offset = 0;
  for(uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++) {
    const auto cmdList = imDrawData->CmdLists[n];
    memcpy(hostVisibleMemory + offset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    offset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
  }
  for(uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++) {
    const auto cmdList = imDrawData->CmdLists[n];
    memcpy(hostVisibleMemory + offset, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    offset += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
  }

  uint32_t indexBufferOffset = uint32_t(bufferOffset + vertexBufferSize);
  ImguiBuffer res = {};
  res.buffer = buffer;
  res.bufferOffset = bufferOffset;
  res.indicesOffset = indexBufferOffset;
  return res;
}

static mg::Pipeline createImguiPipeline(const RenderContext &renderContext) {
  using namespace mg::shaders;
  using namespace mg;


  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
  pipelineStateDesc.graphics.subpass = renderContext.subpass;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = "imgui";
  createPipelineInfo.vertexInputState = imgui::InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputState[2].format = VK_FORMAT_R8G8B8A8_UNORM;
  createPipelineInfo.vertexInputState[2].size = 4;
  createPipelineInfo.vertexInputStateCount = mg::countof(imgui::InputAssembler::vertexInputState);

  auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

static void renderCommandList(const ImguiBuffer &imguiBuffer, const mg::Pipeline &pipeline, const mg::TextureId &fontTextureId) {
  ImDrawData *imDrawData = ImGui::GetDrawData();

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = imDrawData->DisplaySize.x;
  viewport.height = imDrawData->DisplaySize.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(mg::vkContext.commandBuffer, 0, 1, &viewport);

  using namespace mg::shaders::imgui;

  using VertexInputData = InputAssembler::VertexInputData;

  float scale[2] = { 2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y };
  float translate[2] = { translate[0] = -1.0f - imDrawData->DisplayPos.x * scale[0], translate[1] = -1.0f - imDrawData->DisplayPos.y * scale[1] };

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic = (Ubo*)mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  dynamic->uScale = glm::vec2{ 2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y };
  dynamic->uTranslate = glm::vec2{ translate[0] = -1.0f - imDrawData->DisplayPos.x * scale[0], translate[1] = -1.0f - imDrawData->DisplayPos.y * scale[1] };

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &imguiBuffer.buffer, &imguiBuffer.bufferOffset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, imguiBuffer.buffer, imguiBuffer.indicesOffset, VK_INDEX_TYPE_UINT16);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets), dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.textureIndex = mg::getTexture2DDescriptorIndex(fontTextureId);
  vkCmdPushConstants(mg::vkContext.commandBuffer, pipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  // Render the command lists:
  int vtxOffset = 0;
  int idxOffset = 0;
  ImVec2 displayPos = imDrawData->DisplayPos;
  for(uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++)
  {
    const ImDrawList* cmdList = imDrawData->CmdLists[n];
    for(uint32_t cmd_i = 0; cmd_i < uint32_t(cmdList->CmdBuffer.Size); cmd_i++) {
      const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
      if(pcmd->UserCallback) {
        pcmd->UserCallback(cmdList, pcmd);
      }
      else {
        // Apply scissor/clipping rectangle
        // FIXME: We could clamp width/height based on clamped min/max values.
        VkRect2D scissor;
        scissor.offset.x = (int32_t)(pcmd->ClipRect.x - displayPos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - displayPos.x) : 0;
        scissor.offset.y = (int32_t)(pcmd->ClipRect.y - displayPos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - displayPos.y) : 0;
        scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
        scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // FIXME: Why +1 here?
        vkCmdSetScissor(mg::vkContext.commandBuffer, 0, 1, &scissor);

        // Draw
        vkCmdDrawIndexed(mg::vkContext.commandBuffer, pcmd->ElemCount, 1, idxOffset, vtxOffset, 0);
      }
      idxOffset += pcmd->ElemCount;
    }
    vtxOffset += cmdList->VtxBuffer.Size;
  }
}

static void addToolTip(const char* desc) {
  if(ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void Imgui::CreateContext() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(float(mg::vkContext.screen.width), float(mg::vkContext.screen.height));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // font texture
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  uint32_t uploadSize = width * height * 4 * sizeof(char);

  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "Font";
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
  createTextureInfo.size = { uint32_t(width), uint32_t(height), 1 };
  createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  createTextureInfo.data = pixels;
  createTextureInfo.sizeInBytes = uploadSize;

  mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

void Imgui::destroy() {
  mg::mgSystem.textureContainer.removeTexture(_fontId);
}

void Imgui::addLog(const char* fmt, ...) IM_FMTARGS(2) {
  int old_size = _log.buffer.size();
  va_list args;
  va_start(args, fmt);
  _log.buffer.appendfv(fmt, args);
  va_end(args);
  for(int new_size = _log.buffer.size(); old_size < new_size; old_size++)
    if(_log.buffer[old_size] == '\n')
      _log.lineOffsets.push_back(old_size);
  _log.scrollToBottom = true;
}

void Imgui::draw(const RenderContext &renderContext, const FrameData &frameData) const {
  drawUI(frameData);

  auto imguiBuffer = allocateImguiBuffer();
  const auto pipeline = createImguiPipeline(renderContext);
  renderCommandList(imguiBuffer, pipeline, _fontId);
}

void Imgui::drawUI(const mg::FrameData &frameData) const {

  ImGuiIO& io = ImGui::GetIO();

  io.DisplaySize = { float(mg::vkContext.screen.width), float(mg::vkContext.screen.height) };
  io.DeltaTime = 1.0f / 60.0f;

  io.MousePos = ImVec2(frameData.mouse.xy.x * mg::vkContext.screen.width, (1.0f - frameData.mouse.xy.y) * mg::vkContext.screen.height);
  io.MouseDown[0] = frameData.mouse.left;

  ImGui::NewFrame();
  enum class MenuIems { Allocations, Console };
  static MenuIems menuIem = MenuIems::Allocations;

  if(ImGui::Begin("Mongoose", nullptr, { 1024.0f, 512.0f }, -1.0f, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar))
  {
    if(ImGui::BeginMenuBar())
    {
      if(ImGui::Button("Allocations")) {
        menuIem = MenuIems::Allocations;
      }
      if(ImGui::Button("Console")) {
        menuIem = MenuIems::Console;
      }
      ImGui::EndMenuBar();
    }

    switch(menuIem) {
    case MenuIems::Allocations:
      drawAllocations();
      break;
    case MenuIems::Console:
      drawLog();
      break;
    }
  }
  ImGui::End();
  ImGui::EndFrame();
  ImGui::Render();
}


void Imgui::drawLog() const {
  ImGui::SameLine();
  bool copy = ImGui::Button("Copy");
  ImGui::SameLine();
  ImGui::Separator();
  ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  if(copy) ImGui::LogToClipboard();
  ImGui::TextUnformatted(_log.buffer.begin());
  if(_log.scrollToBottom)
    ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
}

enum class TYPE { FIRST_FIT, LINEAR };
static void drawAllocation(const GuiAllocation guiElement, const char *title, TYPE type) {
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
  const auto elements = guiElement.elements;
  const auto sizeNotUsed = elements.back().free ? elements.back().size : 0;
  if(title != nullptr) {
    if(type == TYPE::FIRST_FIT) {
      ImGui::Text(title);
      ImGui::Text("Memory type index: %d, array Index: %d, total size: %.3f mb, used: %0.3f mb, not used: %.3f mb, total allocations: %d, allocations in use: %d", guiElement.memoryTypeIndex, guiElement.heapIndex,
        guiElement.totalSize / 1024.0f / 1024.0f, (guiElement.totalSize - sizeNotUsed) / 1024.0f / 1024.0f, sizeNotUsed / 1024.0f / 1024.0f, guiElement.totalNrOfAllocation, guiElement.allocationNotFreed);
    }
    else {
      ImGui::Text(title);
      ImGui::Text("Memory type index: %d, total size: %.3f mb, used: %0.3f mb, not used: %.3f mb", guiElement.memoryTypeIndex,
        guiElement.totalSize / 1024.0f / 1024.0f, (guiElement.totalSize - sizeNotUsed) / 1024.0f / 1024.0f, sizeNotUsed / 1024.0f / 1024.0f);
    }
  }
  for(uint32_t j = 0; j < elements.size(); j++) {
    if(guiElement.showFullSize == false && j == (elements.size() - 1) && elements[j].free == true)
      continue;
    const auto element = elements[j];
    if(element.free)
      ImGui::PushStyleColor(ImGuiCol_Button, { 0,0.44f,0,1 });
    else
      ImGui::PushStyleColor(ImGuiCol_Button, { 0.44f,0,0,1 });

    auto totalSize = guiElement.showFullSize ? guiElement.totalSize : guiElement.totalSize - sizeNotUsed;
    auto width = mg::vkContext.screen.width * 0.6f;
    auto percent = element.size / float(totalSize);
    char text[20];
    snprintf(text, sizeof(text), "%0.0f kb", element.size / 1024.0f);

    ImGui::Button(text, { percent * width, 20 });

    char toolTip[40];
    snprintf(toolTip, sizeof(toolTip), "Offset: %d, size: %0.3f kb", element.offset, element.size / 1024.0f);

    addToolTip(toolTip);
    if(j < (elements.size() - 1))
      ImGui::SameLine();
    ImGui::PopStyleColor(1);
  }
  ImGui::PopStyleVar(1);
}

void Imgui::drawAllocations() const {
  ImGui::Separator();
  ImGui::Text("Device only first fit allocations:");
  ImGui::Separator();
  {
    // mesh
    auto guiElements = mg::mgSystem.meshDeviceMemoryAllocator.getAllocationForGUI();
    for(auto &guiElement : guiElements) {
      ImGui::Separator();
      if(guiElement.largeDeviceAllocation) {
        drawAllocation(guiElement, "Large size mesh(No heap)", TYPE::LINEAR);
      }
      else {
        guiElement.showFullSize = true;
        drawAllocation(guiElement, "Meshes", TYPE::FIRST_FIT);
        ImGui::Separator();
        guiElement.showFullSize = false;
        drawAllocation(guiElement, nullptr, TYPE::FIRST_FIT);
      }
    }
  }
  ImGui::Separator();
  {
    // texture
    auto guiElements = mg::mgSystem.textureDeviceMemoryAllocator.getAllocationForGUI();
    for(auto &guiElement : guiElements) {
      ImGui::Separator();
      if(guiElement.largeDeviceAllocation) {
        drawAllocation(guiElement, "Large size texture(No heap)", TYPE::LINEAR);
      }
      else {
        guiElement.showFullSize = true;
        drawAllocation(guiElement, "Textures", TYPE::FIRST_FIT);
        ImGui::Separator();
        guiElement.showFullSize = false;
        drawAllocation(guiElement, nullptr, TYPE::FIRST_FIT);
      }
    }
  }
  ImGui::Separator();
  ImGui::Text("Linear allocations:");
  ImGui::Separator();
  {
    // linear
    const auto guiElements = mg::mgSystem.linearHeapAllocator.getAllocationForGUI();
    for(const auto &guiElement : guiElements) {
      ImGui::Separator();
      drawAllocation(guiElement, guiElement.type.c_str(), TYPE::LINEAR);
    }
  }
  ImGui::Separator();
  ImGui::Separator();
  ImGui::Text("");
  ImGui::Text("GPU info:");
  uint32_t offset = 0;
  for(uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryHeapCount; i++) {
    const std::string str = std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryHeaps[i].size / 1024.0f / 1024.0f / 1024.0f) + " Gig";
    ImGui::BulletText(str.c_str());
  }
  ImGui::Separator();
  {
    ImGui::Text("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
    auto flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    for(uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryTypeCount; i++) {
      if((mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
        const std::string str = "Heap index: " + std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].heapIndex) + ", memory index: " + std::to_string(i);
        ImGui::BulletText(str.c_str());
      }
    }
  }

  ImGui::Separator();
  {
    ImGui::Text("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for(uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryTypeCount; i++) {
      if((mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
        const std::string str = "Heap index: " + std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].heapIndex) + ", memory index: " + std::to_string(i);
        ImGui::BulletText(str.c_str());
      }
    }
  }
  ImGui::Separator();
}

} // mg namespace
