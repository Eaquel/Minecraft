cmake_minimum_required(VERSION 4.3.2)
project(omnicraft VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE
        "-O3 -DNDEBUG -ffast-math -funroll-loops -fomit-frame-pointer \
         -fvectorize -fslp-vectorize -fmerge-all-constants \
         -fno-exceptions-unwind-tables")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG -fno-omit-frame-pointer -fsanitize=address")
endif()

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -march=armv8.2-a+dotprod+crypto+fp16 \
        -mtune=cortex-a76 \
        -mfpu=neon-fp-armv8")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -march=armv7-a+neon \
        -mfpu=neon-vfpv4 \
        -mfloat-abi=softfp")
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -march=x86-64-v3 \
        -msse4.2 -mavx2 -mfma")
endif()

add_library(omnicraft SHARED Core.cpp)

find_library(LIB_ANDROID  android)
find_library(LIB_LOG      log)
find_library(LIB_GLES3    GLESv3)
find_library(LIB_EGL      EGL)
find_library(LIB_ZLIB     z)
find_library(LIB_ATOMIC   atomic)

target_link_libraries(omnicraft
    PRIVATE
        ${LIB_ANDROID}
        ${LIB_LOG}
        ${LIB_GLES3}
        ${LIB_EGL}
        ${LIB_ZLIB}
        m
        c++_shared
)

target_compile_definitions(omnicraft PRIVATE
    CHUNK_SIZE=16
    WORLD_HEIGHT=256
    RENDER_DISTANCE=10
    MESH_RENDER_DISTANCE=9
    MAX_PARTICLES=1024
    INVENTORY_SLOTS=36
    HOTBAR_SLOTS=9
    MAX_STACK=64
    ATLAS_DIM=16
    OMNICRAFT_VERSION_MAJOR=2
    OMNICRAFT_VERSION_MINOR=0
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:DEBUG>
)

target_include_directories(omnicraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(omnicraft PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-deprecated-declarations
    $<$<CONFIG:Release>:-Werror>
)

set_target_properties(omnicraft PROPERTIES
    CXX_STANDARD            26
    CXX_STANDARD_REQUIRED   ON
    CXX_EXTENSIONS          OFF
    ANDROID_EXPORT_PACKAGE_SYMBOLS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET omnicraft APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--gc-sections \
          -Wl,--strip-all \
          -Wl,-Bsymbolic \
          -Wl,--exclude-libs,ALL \
          -Wl,--icf=all")
endif()
