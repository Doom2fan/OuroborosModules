cmake_minimum_required(VERSION 3.16)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(OuroborosModules VERSION 1.0 LANGUAGES C CXX)

message(STATUS "${PROJECT_NAME} Build Process" )
message(STATUS "Installing into '${CMAKE_INSTALL_PREFIX}'")
set(BASE_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PLUGIN_NAME ${PROJECT_NAME})
set(ADDITIONAL_PLUGIN_DISTRIBUTABLES LICENSE.md license)

include(RackSDK.cmake)
include(cmake/utils.cmake)

util_add_lib_subdirectory(fmt)
target_compile_definitions(fmt INTERFACE FMT_ENFORCE_COMPILE_STRING=1)
util_add_lib_subdirectory(vcv-rackthemer)

source_group("Module Core" REGULAR_EXPRESSION "src/*")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

target_include_directories(${RACK_PLUGIN_LIB} PRIVATE
    libs/header_only_libs
)
target_sources(${RACK_PLUGIN_LIB} PRIVATE ${SOURCE_FILES})
target_link_libraries(${RACK_PLUGIN_LIB} PRIVATE
    fmt::fmt
    vcv-rackthemer
)

message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET: ${CMAKE_OSX_DEPLOYMENT_TARGET}")