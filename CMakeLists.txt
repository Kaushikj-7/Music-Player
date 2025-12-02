cmake_minimum_required(VERSION 3.16)

project(MusicPlayer
    VERSION 1.0
    DESCRIPTION "Modular C++17 Music Player"
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

# ---------------------------------------------------------
# Output binary directory
# ---------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------
# Folder paths
# ---------------------------------------------------------
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(PLAYER_DIR ${SRC_DIR}/player)
set(DECODER_DIR ${SRC_DIR}/decoder)
set(AUDIO_DIR ${SRC_DIR}/audio)
set(UTILS_DIR ${SRC_DIR}/utils)

# ---------------------------------------------------------
# pkg-config setup
# ---------------------------------------------------------
find_package(PkgConfig REQUIRED)

# FFmpeg
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWR REQUIRED libswresample)

# PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# ---------------------------------------------------------
# Include and link directories
# ---------------------------------------------------------
include_directories(
    ${PLAYER_DIR}
    ${DECODER_DIR}
    ${AUDIO_DIR}
    ${UTILS_DIR}

    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWR_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
)

link_directories(
    ${AVFORMAT_LIBRARY_DIRS}
    ${AVCODEC_LIBRARY_DIRS}
    ${AVUTIL_LIBRARY_DIRS}
    ${SWR_LIBRARY_DIRS}
    ${PORTAUDIO_LIBRARY_DIRS}
)

# ---------------------------------------------------------
# Source files
# ---------------------------------------------------------
set(SOURCES
    ${SRC_DIR}/main.cpp

    ${PLAYER_DIR}/player.cpp
    ${PLAYER_DIR}/player.h

    ${DECODER_DIR}/ffmpeg_decoder.cpp
    ${DECODER_DIR}/ffmpeg_decoder.h

    ${SRC_DIR}/audio/audio_output.cpp
    ${SRC_DIR}/audio/audio_output.h

    ${UTILS_DIR}/logger.cpp
    ${UTILS_DIR}/logger.h
)

# ---------------------------------------------------------
# Executable
# ---------------------------------------------------------
add_executable(music_player ${SOURCES})

# ---------------------------------------------------------
# Link libraries
# ---------------------------------------------------------
target_link_libraries(music_player PRIVATE
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWR_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
)

# Fallback manual linking for PortAudio if pkg-config fails
find_library(PORTAUDIO_LIB portaudio)

if(PORTAUDIO_LIB)
    target_link_libraries(music_player PRIVATE ${PORTAUDIO_LIB})
endif()

# ---------------------------------------------------------
# Install
# ---------------------------------------------------------
install(TARGETS music_player DESTINATION bin)
