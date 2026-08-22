set(PROJECT_NAME "sokketter-ui")
project(${PROJECT_NAME} LANGUAGES CXX)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(PLATFORM_OS_MACOS)
    set(MACOSX_BUNDLE TRUE)
endif()

find_package(Threads REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sources/Qt)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libsokketter/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/ClickableLabel)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/QLedLabel)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/SlidingStackedWidget)

#
# Stating common details.
#
string(TIMESTAMP BUILD_YEAR "%Y")

set(SOKKETTER_COMPANY "Paul Ergård")
set(SOKKETTER_DESCRIPTION "sokketter-ui")

set(SOKKETTER_NAME "sokketter-ui")

set(SOKKETTER_COPYRIGHT "${BUILD_YEAR} Paul Ergård")

if(PLATFORM_OS_WINDOWS)
set(SOKKETTER_EXECUTABLE "sokketter-ui.exe")
else()
set(SOKKETTER_EXECUTABLE "sokketter-ui")
endif()

set(SOKKETTER_BINARY_TYPE 0x3fL)

configure_file(${PROJECT_ROOT_PATH}/version.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.rc @ONLY)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp *.ui *.qrc)

if(PLATFORM_OS_MACOS)
    #
    # Add Objective-C files for MacOS separately.
    #
    list(APPEND FILE_EXTENSIONS *.mm)
elseif(PLATFORM_OS_WINDOWS)
    #
    # Add .rc files for Windows separately.
    #
    list(APPEND FILE_EXTENSIONS *.rc)
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

if(PLATFORM_OS_MACOS)
    #
    # MacOS: state that application has an icon.
    #
    set(MACOSX_BUNDLE_ICON_FILE socket-icon.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/socket-icon.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${APP_ICON_MACOS})

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist
    )

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_FOLDER_FILES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_FOLDER_FILES})
endif()

if(PLATFORM_OS_MACOS)
    #
    # MacOS: state that application has an icon.
    #
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist"
    )

elseif(PLATFORM_OS_WINDOWS)
    #
    # Windows OS: state that this is a GUI application to avoid terminal pop-up.
    #
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

sokketter_set_output_directories(${PROJECT_NAME})

#
# Link with sokketter dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Concurrent Threads::Threads ${SOKKETTER_LIBRARIES})
foreach (SOKKETTER_DEPENDENCY IN LISTS SOKKETTER_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${SOKKETTER_DEPENDENCY})
endforeach()
