cmake_minimum_required(VERSION 3.16.3)
project(simRRS1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

find_package(Boost REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

if(NOT COPPELIASIM_INCLUDE_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(COPPELIASIM_INCLUDE_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/include)
    else()
        set(COPPELIASIM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH ${COPPELIASIM_INCLUDE_DIR}/cmake)
find_package(CoppeliaSim 4.5.1.7 REQUIRED)

coppeliasim_add_plugin(
    simRRS1
    LEGACY
    SOURCES
    sourceCode/simRRS1.cpp
    sourceCode/inputOutputBlock.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simLib/scriptFunctionData.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simLib/scriptFunctionDataItem.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simLib/socketOutConnection.cpp
)
target_link_libraries(simRRS1 PRIVATE Qt5::Core)
coppeliasim_add_lua(lua/simRRS1.lua)
coppeliasim_add_lua(lua/simRRS1-ce.lua)
