set(PROJECT_NAME "sokketter-cli")
project(${PROJECT_NAME} LANGUAGES CXX)

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)

#
# Stating common details.
#
set(SOKKETTER_NAME "sokketter-cli")

if(PLATFORM_OS_WINDOWS)
set(SOKKETTER_EXECUTABLE "sokketter-cli.exe")
else()
set(SOKKETTER_EXECUTABLE "sokketter-cli")
endif()

set(SOKKETTER_COMPANY "")
set(SOKKETTER_COPYRIGHT "")
set(SOKKETTER_DESCRIPTION "A CLI app that provides a simple and easy-to-use way of controlling the connected sockets.")
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

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SOKKETTER_BINARY_OUTPUT_PATH}
    LIBRARY_OUTPUT_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH}
    ARCHIVE_OUTPUT_DIRECTORY ${SOKKETTER_LIBRARY_OUTPUT_PATH}
)

if(PLATFORM_OS_WINDOWS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND cmd.exe /C "move \"${SOKKETTER_LIBRARY_OUTPUT_PATH}\\*.exe\" \"${SOKKETTER_BINARY_OUTPUT_PATH}\""
        COMMENT "Copying all executables to binary output directory"
    )
endif()

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
