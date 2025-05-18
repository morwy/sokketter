#
# Required CMake version.
#
cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.10)

project(sokketter LANGUAGES CXX)

#
# Required C++ standard.
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Project version.
#
include(ProjectVersion.cmake)

#
# Convenience definitions.
#
include(ConvenienceDefinitions.cmake)

#
# Tests.
#
option(SOKKETTER_ENABLE_TESTING "Enable testing" OFF)
option(SOKKETTER_ENABLE_COVERAGE "Enable coverage reporting" OFF)

#
# Third-party fetch.
#
include(ThirdPartyFetch.cmake)

#
# Set output folder for binaries.
#
set(PROJECT_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(SOKKETTER_INCLUDE_OUTPUT_PATH ${PROJECT_ROOT_PATH}/bin/${PLATFORM_OS}_${PLATFORM_TYPE}/${CMAKE_BUILD_TYPE}/include)
set(SOKKETTER_LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/bin/${PLATFORM_OS}_${PLATFORM_TYPE}/${CMAKE_BUILD_TYPE}/libs)
set(SOKKETTER_BINARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/bin/${PLATFORM_OS}_${PLATFORM_TYPE}/${CMAKE_BUILD_TYPE}/bin)

get_property(IS_MULTI_CONFIGURATION_GENERATOR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIGURATION_GENERATOR)
    message("Detected multi-configuration generator, modifying its output directory paths.")
    foreach(CONFIGURATION_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIGURATION_TYPE} CONFIGURATION_TYPE)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIGURATION_TYPE} ${SOKKETTER_LIBRARY_OUTPUT_PATH})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIGURATION_TYPE} ${SOKKETTER_LIBRARY_OUTPUT_PATH})
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION_TYPE} ${SOKKETTER_LIBRARY_OUTPUT_PATH})
    endforeach(CONFIGURATION_TYPE CMAKE_CONFIGURATION_TYPES)

else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SOKKETTER_BINARY_OUTPUT_PATH})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SOKKETTER_BINARY_OUTPUT_PATH})
endif()

message("Creating ${SOKKETTER_INCLUDE_OUTPUT_PATH} build directory.")
file(MAKE_DIRECTORY ${SOKKETTER_INCLUDE_OUTPUT_PATH})

message("Creating ${SOKKETTER_LIBRARY_OUTPUT_PATH} build directory.")
file(MAKE_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH})

message("Creating ${SOKKETTER_BINARY_OUTPUT_PATH} build directory.")
file(MAKE_DIRECTORY ${SOKKETTER_BINARY_OUTPUT_PATH})

#
# State global flags.
#
if(NOT DEFINED IS_COMPILING_STATIC AND NOT DEFINED IS_COMPILING_SHARED)
    set(IS_COMPILING_STATIC true)
    set(IS_COMPILING_SHARED false)
endif()

if(IS_COMPILING_STATIC)
    #
    # Injecting IS_COMPILING_STATIC into build definitions to disable __delcspec() exports under Windows.
    #
    add_definitions(-DIS_COMPILING_STATIC)
endif()

#
# State global include libraries.
#
include_directories(${PROJECT_ROOT_PATH})
include_directories(${PROJECT_ROOT_PATH}/third-party/)

#
# Add projects.
#
add_subdirectory(libsokketter)
# add_subdirectory(libsokketter-test-app)
add_subdirectory(sokketter-cli)
add_subdirectory(sokketter-ui)
