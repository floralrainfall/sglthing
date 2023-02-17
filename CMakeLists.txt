cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)

find_package(glfw3)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ODE ode)

if(NOT glfw3_FOUND)
    add_subdirectory(deps/glfw)
endif()
set(ODE_LIBRARY ${ODE_LIBRARIES})
if(NOT ODE_FOUND)
    add_subdirectory(deps/ODE)
    set(ODE_LIBRARY ODE)
endif()
add_subdirectory(deps/assimp)
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

target_link_libraries(sglthing PUBLIC
    glfw
    glad
    cglm
    assimp
    m
    ${ODE_LIBRARY}
)

target_compile_options(sglthing PUBLIC
    -g)