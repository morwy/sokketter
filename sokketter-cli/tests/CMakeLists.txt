set(PROJECT_NAME "sokketter-cli-tests")
project(${PROJECT_NAME} LANGUAGES CXX)

#
# Add test source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(TEST_FOLDER_FILES ${TEST_FOLDER_FILES} ${FOUND_FILES})
endforeach()

list(FILTER PROJECT_FOLDER_FILES EXCLUDE REGEX ".*main\\.cpp.*")

add_executable(${PROJECT_NAME} ${TEST_FOLDER_FILES} ${PROJECT_FOLDER_FILES} ${GTEST_FILES})

#
# The parent directory no longer leaks its include directories to this subdirectory, so the test
# target needs the same include paths as the sokketter-cli target to compile cli_parser.cpp and the
# test sources. CLI_INCLUDE_DIRS is inherited from the parent scope.
#
target_include_directories(${PROJECT_NAME} PRIVATE ${CLI_INCLUDE_DIRS})

#
# Link with sokketter dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE  gtest gtest_main ${SOKKETTER_LIBRARIES})
foreach (SOKKETTER_DEPENDENCY IN LISTS SOKKETTER_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${SOKKETTER_DEPENDENCY})
endforeach()

if(SOKKETTER_ENABLE_COVERAGE AND NOT PLATFORM_OS_WINDOWS)
    message(STATUS "Enabling coverage flags")

    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_libraries(${PROJECT_NAME} PRIVATE --coverage)

    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_libraries(${PROJECT_NAME} PRIVATE --coverage)
endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)
