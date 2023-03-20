cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_COMMIT_COUNT)

set(ASSIMP_HUNTER_ENABLED FALSE)

find_package(glfw3)
find_package(cglm)
find_package(CURL)
find_package(assimp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 libavutil libavcodec libavformat openal)
#pkg_check_modules(ODE ode)

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
if(NOT CURL_FOUND)
    message(STATUS "curl not found, using deps/curl")
    add_subdirectory(deps/curl)
endif()

project(sglthing)

add_executable(sglthing
    sglthing/s7/script_networking.h
    sglthing/s7/script_networking.c
    sglthing/s7/script_functions.h
    sglthing/s7/script_functions.c
    sglthing/s7/transform.h
    sglthing/s7/transform.c
    sglthing/s7/netbundle.h
    sglthing/s7/netbundle.c
    sglthing/s7/vector.h
    sglthing/s7/vector.c
    sglthing/s7/s7.h
    sglthing/s7/s7.c

    sglthing/primitives.h
    sglthing/primitives.c 
    sglthing/animator.h
    sglthing/animator.c
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
    sglthing/script.h
    sglthing/script.c 
    sglthing/config.h
    sglthing/config.c 
    sglthing/world.h
    sglthing/world.c 
    sglthing/light.h
    sglthing/light.c 
    sglthing/model.h
    sglthing/model.c 
    sglthing/bone.h
    sglthing/bone.c 
    sglthing/http.h
    sglthing/http.c 
    sglthing/net.h
    sglthing/net.c 
    sglthing/snd.h
    sglthing/snd.c 
    sglthing/ui.h
    sglthing/ui.c 
    sglthing/io.h
    sglthing/io.c 
)

target_compile_definitions(sglthing PRIVATE SGLTHING_COMPILE)
configure_file(sglthing/_sglthing.h.in _sglthing.h @ONLY)

target_include_directories(sglthing PUBLIC
    ${ODE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(sglthing PUBLIC
    glfw
    glad
    cglm
    assimp
    m
    ${ZLIB_LIBRARIES} # from assimp
    PkgConfig::deps
    ${CURL_LIBRARIES}
    ${ODE_LIBRARY}
)

target_compile_options(sglthing PUBLIC
    -g)