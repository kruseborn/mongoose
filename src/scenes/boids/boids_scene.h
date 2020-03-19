#pragma once

namespace mg {
struct FrameData;
}

namespace scene {
static const int windowWidth = 1600;
static const int windowHeight = 1600;
}

void initScene();
void destroyScene();
void updateScene(const mg::FrameData &frameData);
void renderScene(const mg::FrameData &frameData);
