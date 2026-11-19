cmake_minimum_required(VERSION 3.24)
project(Engine LANGUAGES C)

find_package(SDL3 REQUIRED)

set(SOURCE_FILES
    ./source/engine/asset_manager.c
    ./source/engine/files.c
    ./source/engine/image.c
    ./source/engine/vector.c
    ./source/engine/face.c
    ./source/engine/mesh.c
    ./source/engine/mesh_extended.c
    ./source/engine/world.c
    ./source/engine/ray.c
    ./source/engine/instances.c
    ./source/engine/render.c
    ./source/engine/main.c
    ./source/world/entry.c
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
)

# Unix-like systems (Linux/macOS) require -lm; Windows (MSVC) includes math functions by default
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()