cmake_minimum_required(VERSION 4.3.2)
project(omnicraft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE
        "-O3 -DNDEBUG -ffast-math -funroll-loops -fomit-frame-pointer \
         -fvectorize -fslp-vectorize -fno-exceptions-unwind-tables \
         -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG -fno-omit-frame-pointer")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+simd -mtune=cortex-a76")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=softfp")
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v2 -msse4.2 -mavx2")
endif()

add_library(omnicraft SHARED
    Core.cpp
)

find_library(ANDROID_LIB  android)
find_library(LOG_LIB      log)
find_library(GLES3_LIB    GLESv3)
find_library(EGL_LIB      EGL)
find_library(OPENSLES_LIB OpenSLES)
find_library(ZLIB_LIB     z)

target_link_libraries(omnicraft
    PRIVATE
        ${ANDROID_LIB}
        ${LOG_LIB}
        ${GLES3_LIB}
        ${EGL_LIB}
        ${OPENSLES_LIB}
        ${ZLIB_LIB}
        m
        atomic
        c++_shared
)

target_compile_definitions(omnicraft PRIVATE
    OMNICRAFT_VERSION_MAJOR=1
    OMNICRAFT_VERSION_MINOR=0
    CHUNK_SIZE=16
    WORLD_HEIGHT=256
    RENDER_DISTANCE=8
    MAX_ENTITIES=512
    MAX_PARTICLES=2048
    INVENTORY_SLOTS=36
    HOTBAR_SLOTS=9
    MAX_STACK=64
    GRAVITY=-28.0f
    JUMP_FORCE=9.5f
    WALK_SPEED=4.317f
    SPRINT_SPEED=5.612f
    SNEAK_SPEED=1.295f
    REACH_DISTANCE=4.5f
    HUNGER_EXHAUSTION_RATE=0.005f
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:DEBUG>
)

target_include_directories(omnicraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(omnicraft PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    $<$<CONFIG:Release>:-Werror>
)

set_target_properties(omnicraft PROPERTIES
    CXX_STANDARD            26
    CXX_STANDARD_REQUIRED   ON
    CXX_EXTENSIONS          OFF
    ANDROID_EXPORT_PACKAGE_SYMBOLS OFF
    LINK_FLAGS              "-Wl,--gc-sections -Wl,--strip-all"
)
