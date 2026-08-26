cmake_minimum_required(VERSION 3.20)

# Build tested on MSVC 2022 x64

project(streamdeck-discordmixer VERSION 2.0.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_IN_STREAMDECK_PLUGINS_DIR "Whether to build to the 'bin' directory or to the stream deck plugins directory." OFF)
set(PLUGIN_OUTPUT_DIR "" CACHE PATH "Optional explicit output directory for the .sdPlugin bundle")

# ===========================================
# Target setup
# ===========================================
set(target streamdeck-discordmixer)

# Qt setup (Qt 6.5 is the oldest supported LTS line.)
find_package(Qt6 6.5 COMPONENTS Core WebSockets Network Gui REQUIRED CONFIG)
set(CMAKE_AUTOMOC ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

if (BUILD_IN_STREAMDECK_PLUGINS_DIR)
    file(TO_CMAKE_PATH "$ENV{APPDATA}" appdata_path)
    set(target_directory "${appdata_path}/Elgato/StreamDeck/Plugins/com.thomast.discordmixer.sdPlugin")
elseif (PLUGIN_OUTPUT_DIR)
    set(target_directory "${PLUGIN_OUTPUT_DIR}")
else ()
    set(target_directory "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/com.thomast.discordmixer.sdPlugin")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${target_directory}/bin")
qt_add_executable(${target})
target_link_libraries(${target} PRIVATE Qt6::WebSockets Qt6::Network Qt6::Gui)
target_compile_definitions(${target} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x060200)

# Embed the marketplace artwork into the Windows executable so Explorer,
# Task Manager and security dialogs show the plugin's own icon.
if (WIN32)
    set(plugin_icon_file "${PROJECT_SOURCE_DIR}/src/plugin/icons/plugin.ico")
    configure_file("${PROJECT_SOURCE_DIR}/src/windows/plugin.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/plugin.rc" @ONLY)
    target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/plugin.rc")
endif ()

if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
else ()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# ===========================================
# Sources
# ===========================================
file(GLOB_RECURSE source_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
target_sources(${target} PRIVATE ${source_files})
target_include_directories(${target} PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/src/deps/qtstreamdeck2"
        "${PROJECT_SOURCE_DIR}/src/deps/qtdiscordipc")

# ===========================================
# Deployment
# ===========================================

# src/plugin contains the manifest, profiles and static plugin assets.
# release/ is reserved exclusively for generated distributable packages.
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/plugin/" DESTINATION ${target_directory})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/deps/qtstreamdeck2/dist/" DESTINATION ${target_directory})

# Run windeployqt
get_target_property(QMAKE_FILEPATH Qt6::qmake IMPORTED_LOCATION)

install(CODE "set(target_file \"$<TARGET_FILE:${target}>\")")
install(CODE "set(QMAKE_FILEPATH \"${QMAKE_FILEPATH}\")")
install(SCRIPT "${PROJECT_SOURCE_DIR}/deploy.cmake")
