cmake_minimum_required(VERSION 3.4)

if(NOT HEADLESS)
    add_subdirectory(glad)
    set(GLAD_LIBRARY glad)
endif()

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_COMMIT_COUNT)

set(ASSIMP_HUNTER_ENABLED FALSE)

if(NOT HEADLESS)
    find_package(glfw3)    
    set(GLFW_LIBRARY glfw)
endif()
find_package(cglm)
find_package(assimp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 libavutil libavcodec libavformat openal zlib)
pkg_check_modules(curl IMPORTED_TARGET libcurl)

option(USE_ODE "Use ODE library" OFF)
option(HEADLESS "Compile without windowing support" OFF)
option(YAAL_COMPILE "Compile YaalOnline" ON)

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

if(NOT HEADLESS)
    if(NOT glfw3_FOUND)
        message(STATUS "glfw3 not found, using deps/glfw")   
        add_subdirectory(deps/glfw)
    endif()
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

if(HEADLESS)
    message(STATUS "compiling headless build")
endif()

project(sglthing)
project(thesglthing)
project(lua)

set(YAAL_COMPILE yes)
if(YAAL_COMPILE)
    project(Yaal)
    add_executable(Yaal
        yaal/netmgr.c
        yaal/netmgr.h
        yaal/yaal.c
        yaal/yaal.h
        yaal/util.c
        yaal/util.h
        yaal/chat.c
        yaal/chat.h
        yaal/map.c
        yaal/map.h
        yaal/fx.c
        yaal/fx.h
    )
    set_target_properties(Yaal PROPERTIES OUTPUT_NAME "thesglyaal")
endif()

file(GLOB LUA_SRC_FILES 
    lua/src/*.c
    lua/src/*.h
)

add_library(lua STATIC
    ${LUA_SRC_FILES}
)

target_include_directories(lua PUBLIC
    lua/src
)

add_library(sglthing STATIC
    sglthing/lua/l_functions.h
    sglthing/lua/l_functions.c
    sglthing/lua/l_vector.h
    sglthing/lua/l_vector.c
    sglthing/primitives.h
    sglthing/primitives.c 
    sglthing/particles.h
    sglthing/particles.c
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
if(HEADLESS)
    target_compile_definitions(sglthing PUBLIC HEADLESS)
endif()

configure_file(sglthing/_sglthing.h.in _sglthinSg.h @ONLY)

target_include_directories(sglthing PUBLIC
    ${ODE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sglthing PUBLIC
    ${GLAD_LIBRARY}
    ${GLFW_LIBRARY}
    cglm
    assimp
    m
    lua
    ${ZLIB_LIBRARIES} # from assimp
    PkgConfig::deps
    PkgConfig::curl
    ${CURL_LIBRARIES}
    ${ODE_LIBRARY}
)

target_compile_options(sglthing PUBLIC
    -g)

target_link_libraries(thesglthing PUBLIC
    sglthing
    ${GLAD_LIBRARY}
)

if(YAAL_COMPILE)
    target_link_libraries(Yaal PUBLIC
        ${GLAD_LIBRARY}
        sglthing
    )
endif()

set(CPACK_PACKAGE_NAME "sglthing")
set(CPACK_PACKAGE_VENDOR "The SGL People")
install(TARGETS sglthing RUNTIME DESTINATION lib)
include(CPack)
