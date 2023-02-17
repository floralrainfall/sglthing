cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)

# im temporarily disabling this because when u cross compiole it doesn't like it

#find_package(glfw3)
#find_package(cglm)
#find_package(assimp)
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(ODE ode)
set(ODE_FOUND OFF)

set(ODE_LIBRARY ${ODE_LIBRARIES})
if(NOT ODE_FOUND)
    message(STATUS "ODE not found, using deps/ODE")   
    add_subdirectory(deps/ODE)
    set(ODE_LIBRARY ODE)
endif()
if(NOT glfw3_FOUND)
    message(STATUS "glfw3 not found, using deps/glfw")   
    add_subdirectory(deps/glfw)
endif()
if(NOT assimp_FOUND)
    message(STATUS "assimp not found, using deps/assimp")   
    add_subdirectory(deps/assimp)
endif()
if(NOT cglm_FOUND)
    message(STATUS "cglm not found, using deps/cglm")
    add_subdirectory(deps/cglm)
endif()

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