cmake_minimum_required(VERSION 3.16)

project(sdf-edit)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native")

file(GLOB_RECURSE SRC_FILES "src/*.h" "src/*.cpp")

include_directories(src)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Compile shaders
message("Setting up shaders...")
add_subdirectory(src/shaders)
add_dependencies(${PROJECT_NAME} Shaders_SPIRV)

# Find packages
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

# Install GLFW
message("Installing GLFW...")
set(GLFW_DIR thirdparty/glfw)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

add_subdirectory(${GLFW_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

set_property(TARGET glfw PROPERTY FOLDER "thirdparty/glfw")

# Install volk
message("Installing volk...")
set(VOLK_DIR thirdparty/volk)

add_subdirectory(${VOLK_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${VOLK_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE volk)

set_property(TARGET volk PROPERTY FOLDER "thirdparty")

# Install ImGui
message("Installing ImGui...")
set(IMGUI_DIR thirdparty/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_DIR})
target_include_directories(imgui PRIVATE ${IMGUI_DIR} ${GLFW_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
set_property(TARGET imgui PROPERTY FOLDER "thirdparty")

# Install fmt
message("Installing FMT...")
set(FMT_DIR thirdparty/fmt)

set(BUILD_DOC OFF CACHE BOOL "")
set(BUILD_TEST OFF CACHE BOOL "")

add_subdirectory(${FMT_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${FMT_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)

# Install VulkanMemoryAllocator
message("Installing VulkanMemoryAllocator...")

set(VMA_DIR thirdparty/VulkanMemoryAllocator)

set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "")
set(VMA_DYNAMIC_VULKAN_FUNCTIONS OFF CACHE BOOL "")

add_subdirectory(${VMA_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${VMA_DIR}/include)
target_include_directories(VulkanMemoryAllocator PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator)

set_property(TARGET VulkanMemoryAllocator PROPERTY FOLDER "thirdparty")

# Install SPIRV-Reflect
message("Installing SPIRV-Reflect...")

set(SPIRV_REFLECT_DIR thirdparty/SPIRV-Reflect)

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "")
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "")
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "")

add_subdirectory(${SPIRV_REFLECT_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SPIRV_REFLECT_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-reflect-static)

set_property(TARGET spirv-reflect-static PROPERTY FOLDER "thirdparty")
