## Mongoose
### Minimalistic Vulkan engine for fast prototyping.
    Windows and Linux(Tested with Arch Linux)
    Support for free type text rendering
    Dear ImGui visualization
    Tightly coupled with files generated from the Shader-Compiler*

#### Dependencies
    freetype-2.6.2
    glfw-3.2.1
    glm
    imgui
    lodepng
    tiny_gltf(will soon be removed)
    
#### Prerequisites
    Environment variable *VULKAN_SDK* has to be set
    Windows, *VS studio* >= 2017
    Linux, *Clang and libc++* >= 8.0
    *cmake* >= 3.12

#### Deferred rendering with SSAO(unzip rungholt.zip before running)
<img src="images/rungholt.png" width="512">

#### Memory allocator visualization using dear IMGUI
<img src="images/mongoose.png" width="512">

#### Physically based rendering from gltf-model
<img src="images/gltf.png" width="512">

#### Isosurface rendering from Volume data
<img src="images/bug.png" width="512">

#### Space invaders
    Instancing with SSBO
    SIMD(avx) for collision detection and translation.
    Collision algorithm: Grid coordinates -> hash map -> linked list in array
    No heap allocation during each frame
<img src="images/space.png" width="512">

##### *http://github.com/kruseborn/shader-compiler
