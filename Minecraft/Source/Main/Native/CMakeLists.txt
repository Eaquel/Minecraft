cmake_minimum_required(VERSION 4.3.2)

project(OmniCraftCore)

add_library(Core SHARED Core.cpp)

find_library(log-lib log)
find_library(GLESv3-lib GLESv3)

target_link_libraries(Core ${log-lib} ${GLESv3-lib})
