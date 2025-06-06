set(PROJECT_NAME "sokketter")
project(${PROJECT_NAME} LANGUAGES CXX C)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

#
# Set include libraries.
#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${PROJECT_ROOT_PATH})

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

#
# Add dependencies.
# Create list of built dependencies and their include directories.
#
if(IS_COMPILING_STATIC)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

if(PLATFORM_OS_MACOS)
    if(IS_COMPILING_SHARED)
        set(MACOSX_RPATH false)
        set(CMAKE_MACOSX_RPATH false)
        set(CMAKE_SKIP_RPATH false)
    endif()
endif()

if (MSVC)
    add_compile_options("/utf-8")
endif()

set(SOKKETTER_LIBRARIES "")
set(SOKKETTER_DEPENDENCIES sokketter)

if(IS_COMPILING_SHARED)
    if(PLATFORM_OS_LINUX)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/libsokketter.so)
    elseif(PLATFORM_OS_MACOS)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/libsokketter.dylib)
    elseif(PLATFORM_OS_WINDOWS)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/sokketter.lib)
    endif()
else()
    if(PLATFORM_OS_LINUX)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/libsokketter.a)
    elseif(PLATFORM_OS_MACOS)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/libsokketter.a)
    elseif(PLATFORM_OS_WINDOWS)
        set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${SOKKETTER_LIBRARY_OUTPUT_PATH}/sokketter.lib)
    endif()
endif()

add_library(${PROJECT_NAME} ${LIBRARY_TYPE} ${PROJECT_FOLDER_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SOKKETTER_BINARY_OUTPUT_PATH}
    LIBRARY_OUTPUT_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH}
    ARCHIVE_OUTPUT_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH}
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/libsokketter.h DESTINATION ${SOKKETTER_INCLUDE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/export_definitions.h DESTINATION ${SOKKETTER_INCLUDE_OUTPUT_PATH})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN_EXPORT)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /MDd)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE /MD)
    endif()
endif()

#
# Link with kommpot dependencies.
#
set(KOMMPOT_INCLUDE_OUTPUT_PATH ${SOKKETTER_INCLUDE_OUTPUT_PATH})
set(KOMMPOT_BINARY_OUTPUT_PATH ${SOKKETTER_LIBRARY_OUTPUT_PATH})
add_subdirectory(${PROJECT_ROOT_PATH}/third-party/kommpot ${PROJECT_ROOT_PATH}/third-party/kommpot)

target_link_libraries(${PROJECT_NAME} PRIVATE ${KOMMPOT_LIBRARIES})
foreach (KOMMPOT_DEPENDENCY IN LISTS KOMMPOT_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${KOMMPOT_DEPENDENCY})
endforeach()

#
# Add kommpot dependencies to in addition the sokketter ones.
#
set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} ${KOMMPOT_LIBRARIES})
set(SOKKETTER_DEPENDENCIES ${SOKKETTER_DEPENDENCIES} ${KOMMPOT_DEPENDENCIES})

#
# Push variables that are local to this file into parent scope.
#
set(SOKKETTER_DEPENDENCIES ${SOKKETTER_DEPENDENCIES} PARENT_SCOPE)
set(SOKKETTER_LIBRARIES ${SOKKETTER_LIBRARIES} PARENT_SCOPE)

message("SOKKETTER_DEPENDENCIES: ${SOKKETTER_DEPENDENCIES}")
message("SOKKETTER_LIBRARIES: ${SOKKETTER_LIBRARIES}")

message("")
