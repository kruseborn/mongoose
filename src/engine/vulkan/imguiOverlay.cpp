#include "imguiOverlay.h"
#include "mg/textureContainer.h"
#include "pipelineContainer.h"
#include "rendering/rendering.h"
#include "vkUtils.h"

#include "mg/mgSystem.h"
#include "mg/tools.h"
#include "mg/window.h"
#include <imgui.h>

namespace mg {

struct ImguiBuffer {
  VkBuffer buffer;
  VkDeviceSize bufferOffset;
  VkDeviceSize indicesOffset;
};

ImguiBuffer allocateImguiBuffer() {
  ImDrawData *imDrawData = ImGui::GetDrawData();
  VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  VkBuffer buffer;
  VkDeviceSize bufferOffset;
  auto hostVisibleMemory = (char *)mg::mgSystem.linearHeapAllocator.allocateBuffer(vertexBufferSize + indexBufferSize,
                                                                                   &buffer, &bufferOffset);

  uint32_t offset = 0;
  for (uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++) {
    const auto cmdList = imDrawData->CmdLists[n];
    memcpy(hostVisibleMemory + offset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    offset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
  }
  for (uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++) {
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
  using namespace mg::shaders::imgui;

  mg::PipelineStateDesc pipelineStateDesc = {};
  pipelineStateDesc.rasterization.vkRenderPass = renderContext.renderPass;
  pipelineStateDesc.rasterization.vkPipelineLayout = mg::vkContext.pipelineLayouts.pipelineLayout;
  pipelineStateDesc.rasterization.rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
  pipelineStateDesc.rasterization.graphics.subpass = renderContext.subpass;

  mg::CreatePipelineInfo createPipelineInfo = {};
  createPipelineInfo.shaderName = shader;
  createPipelineInfo.vertexInputState = InputAssembler::vertexInputState;
  createPipelineInfo.vertexInputState[2].format = VK_FORMAT_R8G8B8A8_UNORM;
  createPipelineInfo.vertexInputState[2].size = 4;
  createPipelineInfo.vertexInputStateCount = mg::countof(InputAssembler::vertexInputState);

  auto pipeline = mg::mgSystem.pipelineContainer.createPipeline(pipelineStateDesc, createPipelineInfo);
  return pipeline;
}

static void renderCommandList(const ImguiBuffer &imguiBuffer, const mg::Pipeline &pipeline,
                              const mg::TextureId &fontTextureId) {
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

  float scale[2] = {2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y};
  float translate[2] = {translate[0] = -1.0f - imDrawData->DisplayPos.x * scale[0],
                        translate[1] = -1.0f - imDrawData->DisplayPos.y * scale[1]};

  VkBuffer uniformBuffer;
  uint32_t uniformOffset;
  VkDescriptorSet uboSet;
  Ubo *dynamic =
      (Ubo *)mgSystem.linearHeapAllocator.allocateUniform(sizeof(Ubo), &uniformBuffer, &uniformOffset, &uboSet);
  dynamic->uScale = glm::vec2{2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y};
  dynamic->uTranslate = glm::vec2{translate[0] = -1.0f - imDrawData->DisplayPos.x * scale[0],
                                  translate[1] = -1.0f - imDrawData->DisplayPos.y * scale[1]};

  vkCmdBindVertexBuffers(mg::vkContext.commandBuffer, 0, 1, &imguiBuffer.buffer, &imguiBuffer.bufferOffset);
  vkCmdBindIndexBuffer(mg::vkContext.commandBuffer, imguiBuffer.buffer, imguiBuffer.indicesOffset,
                       VK_INDEX_TYPE_UINT16);

  vkCmdBindPipeline(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
  DescriptorSets descriptorSets = {};
  descriptorSets.ubo = uboSet;
  descriptorSets.textures = mg::getTextureDescriptorSet();

  uint32_t dynamicOffsets[] = {uniformOffset, 0};
  vkCmdBindDescriptorSets(mg::vkContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          mg::countof(descriptorSets.values), descriptorSets.values, mg::countof(dynamicOffsets),
                          dynamicOffsets);

  TextureIndices textureIndices = {};
  textureIndices.textureIndex = mg::getTexture2DDescriptorIndex(fontTextureId);
  vkCmdPushConstants(mg::vkContext.commandBuffer, pipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(TextureIndices),
                     &textureIndices);

  // Render the command lists:
  int vtxOffset = 0;
  int idxOffset = 0;
  ImVec2 displayPos = imDrawData->DisplayPos;
  for (uint32_t n = 0; n < uint32_t(imDrawData->CmdListsCount); n++) {
    const ImDrawList *cmdList = imDrawData->CmdLists[n];
    for (uint32_t cmd_i = 0; cmd_i < uint32_t(cmdList->CmdBuffer.Size); cmd_i++) {
      const ImDrawCmd *pcmd = &cmdList->CmdBuffer[cmd_i];
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmdList, pcmd);
      } else {
        // Apply scissor/clipping rectangle
        // FIXME: We could clamp width/height based on clamped min/max values.
        VkRect2D scissor;
        scissor.offset.x =
            (int32_t)(pcmd->ClipRect.x - displayPos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - displayPos.x) : 0;
        scissor.offset.y =
            (int32_t)(pcmd->ClipRect.y - displayPos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - displayPos.y) : 0;
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


void Imgui::CreateContext() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(float(mg::vkContext.screen.width), float(mg::vkContext.screen.height));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // font texture
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  uint32_t uploadSize = width * height * 4 * sizeof(char);

  mg::CreateTextureInfo createTextureInfo = {};
  createTextureInfo.id = "Font";
  createTextureInfo.type = mg::TEXTURE_TYPE::TEXTURE_2D;
  createTextureInfo.size = {uint32_t(width), uint32_t(height), 1};
  createTextureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  createTextureInfo.data = pixels;
  createTextureInfo.sizeInBytes = uploadSize;

  mg::mgSystem.textureContainer.createTexture(createTextureInfo);
}

void Imgui::destroy() { mg::mgSystem.textureContainer.removeTexture(_fontId); }

void Imgui::draw(const RenderContext &renderContext, const FrameData &frameData, DrawFunc *drawFunc, void *data) const {
  drawFunc(frameData, data);

  auto imguiBuffer = allocateImguiBuffer();
  const auto pipeline = createImguiPipeline(renderContext);
  renderCommandList(imguiBuffer, pipeline, _fontId);
}

} // namespace mg
