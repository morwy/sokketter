set(PROJECT_NAME "sokketter-ui")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Threads REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp *.ui *.qrc)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel/QLedLabel.h ${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel/QLedLabel.cpp)

set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget/SlidingStackedWidget.h ${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget/SlidingStackedWidget.cpp)

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
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Threads::Threads ${SOKKETTER_LIBRARIES})
foreach (SOKKETTER_DEPENDENCY IN LISTS SOKKETTER_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${SOKKETTER_DEPENDENCY})
endforeach()
