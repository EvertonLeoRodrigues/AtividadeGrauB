cmake_minimum_required(VERSION 3.31)
project(GBatividade)

set(CMAKE_CXX_STANDARD 20)


if (POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif ()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(CMAKE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(CMAKE_ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)
set(GLAD_C_FILE ${CMAKE_SOURCE_DIR}/include/glad/glad.c)

add_executable(GBatividade main.cpp ${GLAD_C_FILE})

target_include_directories(GBatividade PRIVATE ${CMAKE_INCLUDE_DIR} ${CMAKE_ASSETS_DIR})

target_link_libraries(GBatividade PRIVATE
        OpenGL::GL
        glfw
        glm::glm
)

add_custom_command(TARGET GBatividade POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
)

add_custom_command(TARGET GBatividade POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders
)