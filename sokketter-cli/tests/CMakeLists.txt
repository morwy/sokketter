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

message("TEST_FOLDER_FILES: ${TEST_FOLDER_FILES}")
message("PROJECT_FOLDER_FILES: ${PROJECT_FOLDER_FILES}")

add_executable(${PROJECT_NAME} ${TEST_FOLDER_FILES} ${PROJECT_FOLDER_FILES} ${GTEST_FILES})

#
# Link with sokketter dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE  gtest gtest_main ${SOKKETTER_LIBRARIES})
foreach (SOKKETTER_DEPENDENCY IN LISTS SOKKETTER_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${SOKKETTER_DEPENDENCY})
endforeach()

if(ENABLE_COVERAGE)
    message(STATUS "Enabling coverage flags")

    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_libraries(${PROJECT_NAME} PRIVATE --coverage)

    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_libraries(${PROJECT_NAME} PRIVATE --coverage)

    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    add_custom_target(coverage
            COMMAND ${LCOV_PATH} --capture --directory ../sources --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.filtered.info
            COMMAND ${GENHTML_PATH} coverage.filtered.info --output-directory coverage_report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
            VERBATIM
    )
endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)
