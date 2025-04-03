cmake_minimum_required(VERSION 3.30.0)

project(hello VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

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
    )

include(cmake/CPM.cmake)

find_package(Vulkan)
target_link_libraries(hello Vulkan::Vulkan)

CPMAddPackage("gh:glfw/glfw#3.4")
target_link_libraries(hello glfw)

CPMAddPackage("gh:g-truc/glm#1.0.1")
target_link_libraries(hello glm)