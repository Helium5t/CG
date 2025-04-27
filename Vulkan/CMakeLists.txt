cmake_minimum_required(VERSION 3.30.0)

project(hello VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)


# add_definitions(
#     -DSTB_IMAGE_IMPLEMENTATION
# )

add_executable(
    hello heliumdebug.cpp
    main.cpp 
    appdebug.cpp 
    device_specs.cpp 
    setup.cpp 
    validation.cpp 
    shaders.cpp 
    sync.cpp
    vertex.cpp
    image.cpp
)

# Adding stb_image which is not CPM friendly
target_include_directories(hello PRIVATE ${CMAKE_SOURCE_DIR}/deps)
# target_compile_definitions(hello PRIVATE STB_IMAGE_IMPLEMENTATION)
# end

include(cmake/CPM.cmake)

find_package(Vulkan)
target_link_libraries(hello Vulkan::Vulkan)

CPMAddPackage("gh:glfw/glfw#3.4")
target_link_libraries(hello glfw)

CPMAddPackage("gh:g-truc/glm#1.0.1")
target_link_libraries(hello glm)