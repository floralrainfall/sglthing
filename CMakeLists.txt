cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)
add_subdirectory(deps/ODE)
add_subdirectory(deps/assimp)
add_subdirectory(deps/glfw)
add_subdirectory(deps/cglm)

project(sglthing)

add_executable(sglthing
sglthing/primitives.h
sglthing/primitives.c 
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
    sglthing/world.h
    sglthing/world.c 
    sglthing/model.h
    sglthing/model.c 
    sglthing/map.h
    sglthing/map.c 
    sglthing/ui.h
    sglthing/ui.c 
)

target_include_directories(sglthing PUBLIC
    ${ODE_INCLUDE_DIRS}
)

target_link_libraries(sglthing PUBLIC
    glfw
    glad
    cglm
    assimp
    m
    ODE
)

target_compile_options(sglthing PUBLIC
    -g)