set(PROJECT_NAME "sokketter-ui")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(PLATFORM_OS_MACOS)
    set(MACOSX_BUNDLE TRUE)
endif()

find_package(Threads REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/ClickableLabel)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp *.ui *.qrc)

if(PLATFORM_OS_MACOS)
    #
    # Add Objective-C files for MacOS separately.
    #
    list(APPEND FILE_EXTENSIONS *.mm)
endif()

foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

#
# Add third-party classes.
#
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/../third-party/ClickableLabel/ClickableLabel.h ${CMAKE_CURRENT_LIST_DIR}/../third-party/ClickableLabel/ClickableLabel.cpp)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel/QLedLabel.h ${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel/QLedLabel.cpp)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget/SlidingStackedWidget.h ${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget/SlidingStackedWidget.cpp)


if(PLATFORM_OS_MACOS)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_FOLDER_FILES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_FOLDER_FILES})
endif()

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
