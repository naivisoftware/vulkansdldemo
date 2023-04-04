cmake_minimum_required(VERSION 3.17)

project(vulkansdldemo)
  
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(vulkansdldemo src/main.cpp)
target_link_libraries(vulkansdldemo SDL2 vulkan)
