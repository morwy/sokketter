set(PROJECT_NAME "sokketter-cli")
project(${PROJECT_NAME} LANGUAGES CXX)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)

#
# Stating common details.
#
string(TIMESTAMP BUILD_YEAR "%Y")

set(SOKKETTER_COMPANY "Paul Ergård")
set(SOKKETTER_DESCRIPTION "sokketter-cli")

set(SOKKETTER_NAME "sokketter-cli")

set(SOKKETTER_COPYRIGHT "${BUILD_YEAR} Paul Ergård")

if(PLATFORM_OS_WINDOWS)
set(SOKKETTER_EXECUTABLE "sokketter-cli.exe")
else()
set(SOKKETTER_EXECUTABLE "sokketter-cli")
endif()

set(SOKKETTER_BINARY_TYPE 0x3fL)

configure_file(${PROJECT_ROOT_PATH}/version.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.rc @ONLY)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)

if(PLATFORM_OS_WINDOWS)
    #
    # Add .rc files for Windows separately.
    #
    list(APPEND FILE_EXTENSIONS *.rc)
endif()

foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/sources/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

add_executable(${PROJECT_NAME} ${PROJECT_FOLDER_FILES})

sokketter_set_output_directories(${PROJECT_NAME})

#
# Link with sokketter dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${SOKKETTER_LIBRARIES})
foreach (SOKKETTER_DEPENDENCY IN LISTS SOKKETTER_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${SOKKETTER_DEPENDENCY})
endforeach()

#
# Add tests if available.
#
if(SOKKETTER_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
