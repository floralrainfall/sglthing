cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)
add_subdirectory(deps/assimp)

find_package(glfw3 REQUIRED)
find_package(cglm REQUIRED)

project(sglthing)

add_executable(sglthing
    sglthing/sglthing.h
    sglthing/sglthing.c
    sglthing/keyboard.h
    sglthing/keyboard.c
    sglthing/texture.h
    sglthing/texture.c 
    sglthing/graphic.h 
    sglthing/graphic.c
    sglthing/shader.h
    sglthing/shader.c 
    sglthing/player.h
    sglthing/player.c 
    sglthing/world.h
    sglthing/world.c 
    sglthing/model.h
    sglthing/model.c 
    sglthing/ui.h
    sglthing/ui.c 
)

target_link_libraries(sglthing
    glfw
    glad
    cglm
    assimp
    m
)

target_compile_options(sglthing PUBLIC
    -g)