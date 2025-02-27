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

add_executable(${PROJECT_NAME} ${TEST_FOLDER_FILES} ${GTestFiles})
target_include_directories(${PROJECT_NAME} PRIVATE ${googletest_SOURCE_DIR}/googletest/include)

target_link_libraries(${PROJECT_NAME} PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)
