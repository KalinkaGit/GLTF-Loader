cmake_minimum_required(VERSION 3.12.0)
project(GLTFLoader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")

include_directories("include")

add_library(GLTFLoader STATIC ${SOURCES})

add_executable(GLTFLoaderTest "test/main.cpp")
target_link_libraries(GLTFLoaderTest GLTFLoader)
target_include_directories(GLTFLoaderTest PRIVATE "include")

add_custom_command(TARGET GLTFLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/release/include
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GLTFLoader> ${CMAKE_SOURCE_DIR}/release/lib/libGLTFLoader.a
)
