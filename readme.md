Vulkan SDL Integration Demo
=======================

# Description

SDL and Vulkan integration test demo.

This demo attempts to create a window and vulkan compatible surface using SDL.
Verified and tested using multiple GPUs under windows.
Should work on every other SDL / Vulkan supported operating system (OSX, Linux, Android).
main() clearly outlines all the specific steps taken to create a vulkan instance, select a device and create a vulkan compatible surface (opaque) that is associated with a window. The demo also verifies that the GPU can render to the surface.
After compilation you should be ready to draw something using Vulkan.

This demo is based on a set of tutorials (mix and match):

- https://developer.tizen.org/development/guides/native-application/graphics/simple-directmedia-layer-sdl/sdl-graphics-vulkan%C2%AE
- https://vulkan-tutorial.com/Drawing_a_triangle/Presentation

## Compilation

- Windows: Place the vulkansdldemo project next to the vulkan SDK (same level)
    - ie sdk: root/vulkan/1.*/...
    - ie dem: root/vulkansdldemo
- Windows: Open the VS solution and compile.
- Windows: Copy the SDL dll from *\Third-Party\Bin\ to the vulkansdldemo output directory
- Windows: Run

- Others: Compile the main.cpp file and link to the vulkan and SDL2 library.
Example: `g++ main.cpp  -lSDL2 -lvulkan`

## Render Engine

The actual Vulkan implementation of our render engine can be found [here](https://github.com/napframework/nap/tree/main/modules/naprender/src), if of interest, including support for multiple windows, render targets, updating of uniforms and samplers at runtime, compilation of GLSL shaders, loading of Geometry, MSAA etc. 
