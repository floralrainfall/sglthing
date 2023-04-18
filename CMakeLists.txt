cmake_minimum_required(VERSION 3.4)

add_subdirectory(glad)

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_COMMIT_COUNT)

set(ASSIMP_HUNTER_ENABLED FALSE)

find_package(glfw3)
find_package(cglm)
find_package(assimp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 libavutil libavcodec libavformat openal zlib)
pkg_check_modules(curl IMPORTED_TARGET libcurl)

set(USE_ODE NO)
set(ODE_ENABLED NO)
if(USE_ODE)
    message(WARNING "experimental ODE support enabled")
    pkg_check_modules(ODE ode)

    set(ODE_LIBRARY ${ODE_LIBRARIES})
    if(NOT ODE_FOUND)
        message(STATUS "ODE not found, using deps/ODE")   
        add_subdirectory(deps/ODE)
        set(ODE_LIBRARY ODE)
    endif()

    set(ODE_ENABLED YES)
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
if(NOT curl_FOUND)
    message(STATUS "curl not found, using deps/curl")
    add_subdirectory(deps/curl)
endif()

project(sglthing)
project(thesglthing)

set(YAAL_COMPILE yes)
if(YAAL_COMPILE)
    project(Yaal)
    add_executable(Yaal
        yaal/yaal.c
        yaal/yaal.h
        yaal/util.c
        yaal/util.h
    )
    set_target_properties(Yaal PROPERTIES OUTPUT_NAME "thesglyaal")
endif()

add_library(sglthing STATIC
    sglthing/s7/s7.c
    sglthing/s7/s7.h
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

    sglthing/primitives.h
    sglthing/primitives.c 
    sglthing/animator.h
    sglthing/animator.c
    sglthing/sglthing.c
    sglthing/sglthing.h
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
    sglthing/mus.h
    sglthing/mus.c 
    sglthing/ui.h
    sglthing/ui.c 
    sglthing/io.h
    sglthing/io.c 
)

add_executable(thesglthing
    thesglthing/thesglthing.c
)

target_compile_definitions(sglthing PUBLIC SGLTHING_COMPILE S7_DEBUGGING)
if(ODE_ENABLED)
    target_compile_definitions(sglthing PUBLIC ODE_ENABLED)
endif()

configure_file(sglthing/_sglthing.h.in _sglthing.h @ONLY)

target_include_directories(sglthing PUBLIC
    ${ODE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sglthing PUBLIC
    glad
    glfw
    cglm
    assimp
    m
    ${ZLIB_LIBRARIES} # from assimp
    PkgConfig::deps
    PkgConfig::curl
    ${CURL_LIBRARIES}
    ${ODE_LIBRARY}
)

target_compile_options(sglthing PUBLIC
    -g)

target_link_libraries(thesglthing PUBLIC
    glad
    sglthing
)

if(YAAL_COMPILE)
    target_link_libraries(Yaal PUBLIC
        glad
        sglthing
    )
endif()

set(CPACK_PACKAGE_NAME "sglthing")
set(CPACK_PACKAGE_VENDOR "The SGL People")
install(TARGETS sglthing RUNTIME DESTINATION lib)
include(CPack)
