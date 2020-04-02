#include "allocationUI.h"
#include "mg/mgSystem.h"
#include <imgui.h>

namespace mg {

static void addToolTip(const char *desc) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

enum class TYPE { FIRST_FIT, LINEAR };
static void drawAllocation(const GuiAllocation guiElement, const char *title, TYPE type) {
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
  const auto elements = guiElement.elements;
  const auto sizeNotUsed = elements.back().free ? elements.back().size : 0;
  if (title != nullptr) {
    if (type == TYPE::FIRST_FIT) {
      ImGui::Text(title);
      ImGui::Text("Memory type index: %d, array Index: %d, total size: %.3f mb, used: %0.3f mb, not used: %.3f mb, "
                  "total allocations: %d, allocations in use: %d",
                  guiElement.memoryTypeIndex, guiElement.heapIndex, guiElement.totalSize / 1024.0f / 1024.0f,
                  (guiElement.totalSize - sizeNotUsed) / 1024.0f / 1024.0f, sizeNotUsed / 1024.0f / 1024.0f,
                  guiElement.totalNrOfAllocation, guiElement.allocationNotFreed);
    } else {
      ImGui::Text(title);
      ImGui::Text("Memory type index: %d, total size: %.3f mb, used: %0.3f mb, not used: %.3f mb",
                  guiElement.memoryTypeIndex, guiElement.totalSize / 1024.0f / 1024.0f,
                  (guiElement.totalSize - sizeNotUsed) / 1024.0f / 1024.0f, sizeNotUsed / 1024.0f / 1024.0f);
    }
  }
  for (uint32_t j = 0; j < elements.size(); j++) {
    if (guiElement.showFullSize == false && j == (elements.size() - 1) && elements[j].free == true)
      continue;
    const auto element = elements[j];
    if (element.free)
      ImGui::PushStyleColor(ImGuiCol_Button, {0, 0.44f, 0, 1});
    else
      ImGui::PushStyleColor(ImGuiCol_Button, {0.44f, 0, 0, 1});

    auto totalSize = guiElement.showFullSize ? guiElement.totalSize : guiElement.totalSize - sizeNotUsed;
    auto width = mg::vkContext.screen.width * 0.6f;
    auto percent = element.size / float(totalSize);
    char text[20];
    snprintf(text, sizeof(text), "%0.0f kb", element.size / 1024.0f);

    ImGui::Button(text, {percent * width, 20});

    char toolTip[40];
    snprintf(toolTip, sizeof(toolTip), "Offset: %d, size: %0.3f kb", element.offset, element.size / 1024.0f);

    addToolTip(toolTip);
    if (j < (elements.size() - 1))
      ImGui::SameLine();
    ImGui::PopStyleColor(1);
  }
  ImGui::PopStyleVar(1);
}

static void drawAllocations() {
  ImGui::Separator();
  ImGui::Text("Device only first fit allocations:");
  ImGui::Separator();
  {
    // mesh
    auto guiElements = mg::mgSystem.meshDeviceMemoryAllocator.getAllocationForGUI();
    for (auto &guiElement : guiElements) {
      ImGui::Separator();
      if (guiElement.largeDeviceAllocation) {
        drawAllocation(guiElement, "Large size mesh(No heap)", TYPE::LINEAR);
      } else {
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
    for (auto &guiElement : guiElements) {
      ImGui::Separator();
      if (guiElement.largeDeviceAllocation) {
        drawAllocation(guiElement, "Large size texture(No heap)", TYPE::LINEAR);
      } else {
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
    for (const auto &guiElement : guiElements) {
      ImGui::Separator();
      drawAllocation(guiElement, guiElement.type.c_str(), TYPE::LINEAR);
    }
  }
  ImGui::Separator();
  ImGui::Separator();
  ImGui::Text("");
  ImGui::Text("GPU info:");
  for (uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryHeapCount; i++) {
    const std::string str =
        std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryHeaps[i].size / 1024.0f / 1024.0f / 1024.0f) +
        " Gig";
    ImGui::BulletText(str.c_str());
  }
  ImGui::Separator();
  {
    ImGui::Text("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
    auto flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    for (uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryTypeCount; i++) {
      if ((mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
        const std::string str =
            "Heap index: " + std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].heapIndex) +
            ", memory index: " + std::to_string(i);
        ImGui::BulletText(str.c_str());
      }
    }
  }

  ImGui::Separator();
  {
    ImGui::Text("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < mg::vkContext.physicalDeviceMemoryProperties.memoryTypeCount; i++) {
      if ((mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
        const std::string str =
            "Heap index: " + std::to_string(mg::vkContext.physicalDeviceMemoryProperties.memoryTypes[i].heapIndex) +
            ", memory index: " + std::to_string(i);
        ImGui::BulletText(str.c_str());
      }
    }
  }
  ImGui::Separator();
}

void drawAllocations(const mg::FrameData &frameData) {
  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = {float(mg::vkContext.screen.width), float(mg::vkContext.screen.height)};
  io.DeltaTime = 1.0f / 60.0f;

  io.MousePos = ImVec2(frameData.mouse.xy.x * mg::vkContext.screen.width,
                       (1.0f - frameData.mouse.xy.y) * mg::vkContext.screen.height);
  io.MouseDown[0] = frameData.mouse.left;

  ImGui::NewFrame();
  enum class MenuIems { Allocations, Console };
  static MenuIems menuIem = MenuIems::Allocations;

  if (ImGui::Begin("Mongoose", nullptr, {1024.0f, 512.0f}, -1.0f,
                   ImGuiWindowFlags_AlwaysAutoResize)) {
      drawAllocations();
  }
  ImGui::End();
  ImGui::EndFrame();
  ImGui::Render();
}
} // namespace mg
