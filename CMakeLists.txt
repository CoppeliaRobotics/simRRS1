cmake_minimum_required(VERSION 3.15)
project(simExtRRS1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

find_package(Boost REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

if(NOT LIBPLUGIN_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(LIBPLUGIN_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/libPlugin)
    else()
        set(LIBPLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libPlugin)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH ${LIBPLUGIN_DIR}/cmake)
find_package(CoppeliaSim 4.5.0.0 REQUIRED)

coppeliasim_add_plugin(
    simExtRRS1
    LEGACY
    SOURCES
    simExtRRS1.cpp
    inputOutputBlock.cpp
    ${COPPELIASIM_INCLUDE_DIR}/scriptFunctionData.cpp
    ${COPPELIASIM_INCLUDE_DIR}/scriptFunctionDataItem.cpp
    ${COPPELIASIM_INCLUDE_DIR}/socketOutConnection.cpp
)
target_link_libraries(simExtRRS1 Qt5::Core)
