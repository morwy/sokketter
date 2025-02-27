set(PROJECT_NAME "sokketter-cli")
project(${PROJECT_NAME} LANGUAGES CXX)

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/sources/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

add_executable(${PROJECT_NAME} ${PROJECT_FOLDER_FILES})

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
add_subdirectory(tests)
