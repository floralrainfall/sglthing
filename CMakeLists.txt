cmake_minimum_required(VERSION 3.4)

if(NOT HEADLESS)
    add_subdirectory(glad)
    set(GLAD_LIBRARY glad)
endif()

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_COMMIT_COUNT)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "configuring sglthing-" ${GIT_BRANCH} " r" ${GIT_COMMIT_COUNT})

set(ASSIMP_HUNTER_ENABLED FALSE)

option(USE_ODE "Use ODE library" OFF)
option(USE_PKGCONFIG "Use PkgConfig to find packages" ON)
option(HEADLESS "Compile without windowing support" OFF)
option(GAMES_COMPILE "Compile the games" ON)
option(SOUND_ENABLED "Enable sound using PortAudio and libsndfile" ON)
option(FBO_ENABLED "Enable frame-buffer objects" ON)
if(NOT HEADLESS)
    option(SOUND_ENABLED "Enable sound" ON)
    if(SOUND_ENABLED)
        option(OPUS_ENABLED "Enable libopus voice chat" ON)
    endif()
    find_package(glfw3)    
    set(GLFW_LIBRARY glfw)
else()
    set(SOUND_ENABLED OFF)
    set(OPUS_ENABLED OFF)
    set(FBO_ENABLED OFF)
endif()
if(USE_PKGCONFIG)
    if(PKG_CONFIG_EXECUTABLE)
        set(RESET_PKG_CONFIG_VALUE ${PKG_CONFIG_EXECUTABLE})
        message(WARNING "PKG_CONFIG_EXECUTABLE being saved " ${PKG_CONFIG_EXECUTABLE})
    endif()
    find_package(PkgConfig)
    if(RESET_PKG_CONFIG_VALUE)
        set(PKG_CONFIG_EXECUTABLE ${RESET_PKG_CONFIG_VALUE})
    endif()
    pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0)
    pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)
    pkg_check_modules(assimp REQUIRED IMPORTED_TARGET assimp)
    pkg_check_modules(cglm REQUIRED IMPORTED_TARGET cglm)
    pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl)
    pkg_check_modules(sqlite REQUIRED IMPORTED_TARGET sqlite3)
    pkg_check_modules(freetype REQUIRED IMPORTED_TARGET freetype2)
    pkg_check_modules(json REQUIRED IMPORTED_TARGET json-c)
    set(PKGCONFIG_LIBRARIES 
        PkgConfig::glib PkgConfig::zlib PkgConfig::assimp PkgConfig::cglm PkgConfig::libcurl PkgConfig::sqlite PkgConfig::freetype PkgConfig::json)
else()
    message(WARNING "you probably wont be able to use glib or zlib")
endif()

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
        message(WARNING "glfw3 not found, using deps/glfw")   
        add_subdirectory(deps/glfw)
    endif()

    if(SOUND_ENABLED)
        pkg_check_modules(sound_deps REQUIRED IMPORTED_TARGET 
            portaudio-2.0 sndfile
        )
        set(PKGCONFIG_LIBRARIES ${PKGCONFIG_LIBRARIES} PkgConfig::sound_deps)
        if(OPUS_ENABLED)
            pkg_check_modules(opus_deps REQUIRED IMPORTED_TARGET 
                opus
            )
            set(PKGCONFIG_LIBRARIES ${PKGCONFIG_LIBRARIES} PkgConfig::opus_deps)
        endif()
    endif()
endif()

if(NOT zlib_FOUND)
    message(WARNING "zlib not found, using deps/zlib")
    add_subdirectory(deps/zlib)
endif()
if(NOT assimp_FOUND)
    message(WARNING "assimp not found, using deps/assimp")   
    if(NOT zlib_FOUND)
        message(WARNING "since zlib not found, ASSIMP_BUILD_ZLIB = ON")   
        set(ASSIMP_BUILD_ZLIB ON)
    endif()
    add_subdirectory(deps/assimp)
endif()
if(NOT cglm_FOUND)
    message(WARNING "cglm not found, using deps/cglm")
    add_subdirectory(deps/cglm)
endif()
if(NOT curl_FOUND)
    message(WARNING "curl not found, using deps/curl")
    add_subdirectory(deps/curl)
endif()
if(HEADLESS)
    message(STATUS "compiling headless build")
endif()

project(sglthing)
project(thesglthing)
project(lua)

if(GAMES_COMPILE)
    project(RDM2)
    add_executable(RDM2
        sglrdm2/objective.c
        sglrdm2/objective.h
        sglrdm2/gamemode.c
        sglrdm2/gamemode.h
        sglrdm2/rdm_net.c
        sglrdm2/rdm_net.h
        sglrdm2/weapons.c
        sglrdm2/weapons.h
        sglrdm2/perlin.c
        sglrdm2/perlin.h
        sglrdm2/rdm2.c
        sglrdm2/rdm2.h
        sglrdm2/map.c
        sglrdm2/map.h
        sglrdm2/ray.c
        sglrdm2/ray.h
    )
    set_target_properties(RDM2 PROPERTIES OUTPUT_NAME "rdm2-ng")
    project(YittlePeople)
    add_executable(YittlePeople
        yittlepeople/yittlepeople.c
        yittlepeople/yittlepeople.h
        yittlepeople/building.c
        yittlepeople/building.h
        yittlepeople/civ.c
        yittlepeople/civ.h
        yittlepeople/guy.c
        yittlepeople/guy.h
    )
    set_target_properties(YittlePeople PROPERTIES OUTPUT_NAME "theyittlepeople")
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
    sglthing/memory.h 
    sglthing/memory.c
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
    sglthing/prof.h
    sglthing/prof.c 
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
if(SOUND_ENABLED)
    target_compile_definitions(sglthing PUBLIC SOUND_ENABLED)
endif()
if(OPUS_ENABLED)
    target_compile_definitions(sglthing PUBLIC OPUS_ENABLED)
endif()
if(FBO_ENABLED)
    target_compile_definitions(sglthing PUBLIC FBO_ENABLED)
endif()

configure_file(sglthing/_sglthing.h.in _sglthing.h @ONLY)

target_include_directories(sglthing PUBLIC
    ${ODE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${USER_INCLUDE_DIR}
)

target_link_libraries(sglthing PUBLIC
    ${GLAD_LIBRARY}
    ${GLFW_LIBRARY}
    cglm ${USER_CGLM_LIBRARY}
    assimp ${USER_ASSIMP_LIBRARY}
    m 
    lua
    ${ZLIB_LIBRARIES} # from assimp
    ${CURL_LIBRARIES}
    ${ODE_LIBRARY}
    ${PKGCONFIG_LIBRARIES}
)

target_compile_options(sglthing PUBLIC
    -g)

target_link_libraries(thesglthing PUBLIC
    sglthing
    ${GLAD_LIBRARY}
)

if(GAMES_COMPILE)
    target_link_libraries(Yaal PUBLIC
        ${GLAD_LIBRARY}
        sglthing
    )
    target_link_libraries(RDM2 PUBLIC
        ${GLAD_LIBRARY}
        sglthing
    )
    target_link_libraries(YittlePeople PUBLIC
        ${GLAD_LIBRARY}
        sglthing
    )
endif()

set(CPACK_PACKAGE_NAME "sglthing")
set(CPACK_PACKAGE_VENDOR "The SGL People")
install(TARGETS sglthing RUNTIME DESTINATION lib)
include(CPack)
