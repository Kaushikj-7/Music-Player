cmake_minimum_required(VERSION 3.16)

project(MusicPlayer
    VERSION 1.0
    DESCRIPTION "Modular C++17 Music Player (Stage 1 Framework)"
    LANGUAGES CXX
)

# ---------------------------------------------------------
# C++ Standard
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Directory mappings
# ---------------------------------------------------------
set(SRC_DIR      ${CMAKE_SOURCE_DIR}/src)
set(PLAYER_DIR   ${SRC_DIR}/player)
set(DECODER_DIR  ${SRC_DIR}/decoder)
set(UTILS_DIR    ${SRC_DIR}/utils)

# ---------------------------------------------------------
# Header include paths
# ---------------------------------------------------------
include_directories(
    ${PLAYER_DIR}
    ${DECODER_DIR}
    ${UTILS_DIR}
)

# ---------------------------------------------------------
# Source files
# ---------------------------------------------------------
set(SOURCES
    ${SRC_DIR}/main.cpp

    ${PLAYER_DIR}/player.cpp
    ${PLAYER_DIR}/player.h

    ${DECODER_DIR}/wav_decoder.cpp
    ${DECODER_DIR}/wav_decoder.h

    ${UTILS_DIR}/logger.cpp
    ${UTILS_DIR}/logger.h
)

# ---------------------------------------------------------
# Output binary
# ---------------------------------------------------------
add_executable(music_player ${SOURCES})

# ---------------------------------------------------------
# Future library linking point
# ---------------------------------------------------------
# target_link_libraries(music_player PRIVATE portaudio)
# target_link_libraries(music_player PRIVATE cuda_runtime)

# ---------------------------------------------------------
# Install target
# ---------------------------------------------------------
install(TARGETS music_player DESTINATION bin)
