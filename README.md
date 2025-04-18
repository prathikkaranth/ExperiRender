# ExperiRender

![image](docs/images/Sponza-RT_Mat.png)

A cross-platform Graphics Renderer written in Vulkan mainly for visualization purposes required in research fields. Current iteration has support for rasterization and ray tracing. 

- Rasterizer
    - Shadow Maps 
    - Screen Space Ambient Occlusion (SSAO) 
    - Mipmaps 
    - Environment Maps (HDRI)
    - Blinn-phong lighting 
    - PBR
- Hardware Ray Tracing

# Build

Build system should work for windows and linux based systems. Build configuration currently tested for Windows 11 (x64) and Ubuntu 24.04 (Noble Numbat)

_Note: Non NVIDIA RTX cards could potentially give errors for the raytracer_

## Linux

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html)
```bash
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt update
sudo apt install vulkan-sdk
```

2. Install SDL 2.0
```bash
sudo apt install libsdl2-2.0-0 libsdl2-dev libsdl-image1.2-dev libsdl1.2-dev
```

3. Clone repository
```
git clone https://github.com/prathikkaranth/ExperiRender.git
```

4. Get assets for [Sponza](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Sponza), [FlightHelmet](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/FlightHelmet) and [HDRI](https://polyhaven.com/a/mirrored_hall) and paste them in the /assets folder. Important to note here that the file structure should be `/assets/Sponza/glTF/{asset_content}`(same for Flight Helmet) and the HDRI should be `/assets/HDRI/mirrored_hall_4k.hdr`

5. Build using CMake

```bash
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_CXX_FLAGS="-std=c++20" -DCMAKE_CXX_SCAN_FOR_MODULES=OFF ..
make
```

## Windows

_Instructions tested on Visual Studio 2022_

1. Download and Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) in your C: drive root (important for shader compilation). Add the installed sdk directory in your environment variables.

2. Clone repository
```
git clone https://github.com/prathikkaranth/ExperiRender.git
```

3. Get assets for [Sponza](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Sponza), [FlightHelmet](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/FlightHelmet) and [HDRI](https://polyhaven.com/a/mirrored_hall) and paste them in the /assets folder. Important to note here that the file structure should be `/assets/Sponza/glTF/{asset_content}`(similar for Flight Helmet) and the HDRI should be `/assets/HDRI/mirrored_hall_4k.hdr`

4. Build using CMake GUI to configure and generate a Visual Studio solution. Set startup project to 'Renderer' and run.


## Showcase

Showcase | Details
---------|--------
![small](docs/images/TheTravelingWagon-RT_NoMat.png) | <b> Ray Traced Geometry, AO </b> <br>Implements gltf geometry loaded with ray tracing using Vulkan Ray Tracing extensions.
![small](docs/images/TheTravelingWagon-SSAO_ON.png) ![small](docs/images/TheTravelingWagon-SSAO_OFF.png) | <b> Screen Space Ambient Occlusion </b> <br>Implements screen space ambient occlusion.
![small](docs/images/ExperiRender_Sponza.png) | <b> Shadow Maps </b> <br>Implements shadow mapping.

## Models Used for Showcase

Model - ["The Traveling Wagon - Cheeeeeeeeeese"](https://sketchfab.com/3d-models/the-traveling-wagon-cheeeeeeeeeese-d9761a00e1f34d7ab4e83cc92e94d6f3) by [Sophie Becker](https://sketchfab.com/sophiebecker) licensed under [CC-BY-4.0](http://creativecommons.org/licenses/by/4.0/)

Model - [Sponza](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Sponza)

## Dependencies Required:

- [Vulkan SDK 1.3](https://vulkan.lunarg.com/sdk/home)
- [SDL](https://github.com/libsdl-org/SDL)
- [GLM](https://github.com/g-truc/glm)
- [VK-Bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [FastGLTF](https://github.com/spnda/fastgltf)
- [DearImGui](https://github.com/ocornut/imgui)
- [spdlog](https://github.com/gabime/spdlog)


