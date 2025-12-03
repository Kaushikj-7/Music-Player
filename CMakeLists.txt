cmake_minimum_required(VERSION 3.16)

project(MusicPlayer
    VERSION 1.0
    DESCRIPTION "Modular C++17 Music Player with GUI"
    LANGUAGES CXX
)

# ---------------------------------------------------------
# C++ Standard
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------
# Dependencies
# ---------------------------------------------------------
include(FetchContent)

# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

find_package(PkgConfig REQUIRED)

# FFmpeg
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWR REQUIRED libswresample)

# PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# SDL2
pkg_check_modules(SDL2 REQUIRED sdl2)

# OpenGL
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------
# Sources
# ---------------------------------------------------------
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# ImGui Sources
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(music_player
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/player/player.cpp
    ${SRC_DIR}/decoder/ffmpeg_decoder.cpp
    ${SRC_DIR}/audio/audio_output.cpp
    ${SRC_DIR}/utils/logger.cpp
    ${IMGUI_SOURCES}
)

# ---------------------------------------------------------
# Include Directories
# ---------------------------------------------------------
target_include_directories(music_player PRIVATE
    ${SRC_DIR}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWR_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ---------------------------------------------------------
# Link Libraries
# ---------------------------------------------------------
target_link_libraries(music_player PRIVATE
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWR_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${SDL2_LIBRARIES}
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

# ---------------------------------------------------------
# Install
# ---------------------------------------------------------
install(TARGETS music_player DESTINATION bin)
