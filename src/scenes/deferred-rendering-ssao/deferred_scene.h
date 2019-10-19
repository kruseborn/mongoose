#pragma once

namespace mg {
struct FrameData;
}

void initScene();
void destroyScene();
void updateScene(const mg::FrameData &frameData);
void renderScene(const mg::FrameData &frameData);
