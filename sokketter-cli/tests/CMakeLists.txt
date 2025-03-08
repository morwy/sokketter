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

    find_program(CTEST_PATH NAMES ctest)
    find_program(LCOV_PATH NAMES lcov)
    find_program(GENHTML_PATH NAMES genhtml)

    if(CTEST_PATH AND LCOV_PATH AND GENHTML_PATH)
        message(STATUS "Coverage tools found: ${CTEST_PATH}, ${LCOV_PATH}, ${GENHTML_PATH}")
        message(STATUS "Target test coverage folder: ${CMAKE_BINARY_DIR}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "-------------------------------------------------------"
            COMMAND ${CMAKE_COMMAND} -E echo "Running the tests..."
            COMMAND ${CTEST_PATH} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E echo "-------------------------------------------------------"
            COMMAND ${CMAKE_COMMAND} -E echo "Building the coverage..."
            COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage.info --keep-going || ${CMAKE_COMMAND} -E true
            COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info '/Applications/*' 'v1/*' '${PROJECT_ROOT_PATH}/third-party/*' '${CMAKE_BINARY_DIR}/_deps/*' '${PROJECT_ROOT_PATH}/sokketter-cli/tests/*' --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info --keep-going || ${CMAKE_COMMAND} -E true
            COMMAND ${CMAKE_COMMAND} -E echo "-------------------------------------------------------"
            COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML report..."
            COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage.filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report --keep-going || ${CMAKE_COMMAND} -E true
        )
    else()
        message(WARNING "Coverage tools not found, skipping coverage generation.")
    endif()
endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)
