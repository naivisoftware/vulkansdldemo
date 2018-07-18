Vulkan SDL Demo {#mainpage}
=======================

# Description

SDL and Vulkan integration test demo.

This demo attempts to create a window and vulkan compatible surface using SDL.
Verified and tested using multiple GPUs under windows.
Should work on every other SDL / Vulkan supported operating system (OSX, Linux, Android)
main() clearly outlines all the specific steps taken to create a vulkan instance, select a device and create a vulkan compatible surface (opaque) that is associated with a window. The demo also verifies that the GPU can render to the surface.
After compilation you should be ready to draw something using Vulkan.

This demo is based on a set of tutorials (mix and match):

- https://developer.tizen.org/development/guides/native-application/graphics/simple-directmedia-layer-sdl/sdl-graphics-vulkan%C2%AE
- https://vulkan-tutorial.com/Drawing_a_triangle/Presentation

## Compilation

- Windows: Download the Vulkan SDK and place it next to the vulkandemo project (same level)
- Windows: Open the VS solution and compile.
- Windows: Copy the sdl dll from *\Third-Party\Bin\ to the bin output directory
- Windows: Run

- Others: simply compile the main.cpp file and link to the vulkan and SDL library.