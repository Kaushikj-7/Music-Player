/**
 * main.cpp
 *
 * GUI Music Player using Dear ImGui + SDL2 + OpenGL3.
 * Features: Playlist, Loop, Speed Control, Sleek Dark Theme.
 */

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>

#include "player/player.h"
#include "utils/logger.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

const std::string PLAYLIST_FILE = "playlist.txt";

// Helper to convert Windows paths (e.g. "C:\Music") to WSL paths (e.g. "/mnt/c/Music")
std::string convertWindowsPathToWSL(std::string path) {
    // Remove surrounding quotes
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }
    // Convert backslashes
    std::replace(path.begin(), path.end(), '\\', '/');
    // Convert drive letter
    if (path.size() >= 2 && path[1] == ':') {
        char drive = tolower(path[0]);
        path = "/mnt/" + std::string(1, drive) + path.substr(2);
    }
    return path;
}

void savePlaylist(const std::vector<std::string>& playlist) {
    std::ofstream out(PLAYLIST_FILE);
    if (out.is_open()) {
        for (const auto& path : playlist) {
            out << path << "\n";
        }
    }
}

void loadPlaylist(std::vector<std::string>& playlist) {
    std::ifstream in(PLAYLIST_FILE);
    std::string line;
    if (in.is_open()) {
        while (std::getline(in, line)) {
            // Only add if file still exists
            if (!line.empty() && fs::exists(line)) {
                playlist.push_back(line);
            }
        }
    }
}

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.85f); // Semi-transparent black
    style.Colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    style.Colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.15f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
    style.Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.40f, 0.40f, 0.40f, 0.60f);
    style.Colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // MX Blue-ish
    style.Colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_ButtonActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    style.Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowRounding = 0.0f; // Sleek, sharp corners
    style.ChildRounding = 0.0f;
    style.FrameRounding = 4.0f; // Soft buttons
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 0.0f;
    
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 8);
}

int main(int argc, char** argv) {
    Logger::instance().setLogFile("music_player_gui.log");
    Logger::instance().log(LogLevel::INFO, "GUI App started");

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        Logger::instance().log(LogLevel::ERROR, "Error: " + std::string(SDL_GetError()));
        return -1;
    }

    // Decide GL+GLSL versions
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Music Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    if (!window) {
        Logger::instance().log(LogLevel::ERROR, "Error: SDL_CreateWindow(): " + std::string(SDL_GetError()));
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    SetupStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Player State
    Player player;
    std::vector<std::string> playlist;
    int currentTrackIndex = -1;
    float volume = 1.0f;
    bool loop = false;
    float speed = 1.0f;

    // Load persistent playlist
    loadPlaylist(playlist);

    // Scan current directory for audio files (Optional: Remove this if you only want persistent playlist)
    // For now, I'll comment it out to strictly follow "remain permanently until i delete it"
    /*
    try {
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                // Simple check for common audio extensions
                if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg") {
                    playlist.push_back(fs::absolute(entry.path()).string());
                }
            }
        }
    } catch (...) {
        Logger::instance().log(LogLevel::ERROR, "Failed to scan directory");
    }
    */
    
    // Remove duplicates and sort
    std::sort(playlist.begin(), playlist.end());
    playlist.erase(std::unique(playlist.begin(), playlist.end()), playlist.end());

    if (argc > 1) {
        // If file provided in args, try to find it in playlist or add it
        std::string argFile = argv[1];
        // Ensure absolute path if possible, though argv[1] from wslpath should be absolute
        if (fs::exists(argFile)) {
             argFile = fs::absolute(argFile).string();
        }

        auto it = std::find(playlist.begin(), playlist.end(), argFile);
        if (it != playlist.end()) {
            currentTrackIndex = std::distance(playlist.begin(), it);
        } else {
            playlist.push_back(argFile);
            currentTrackIndex = playlist.size() - 1;
            savePlaylist(playlist); // Save immediately
        }
        
        if (player.load(playlist[currentTrackIndex])) {
            player.play();
        }
    } else if (!playlist.empty()) {
        // If no args but playlist exists, load first track but don't auto-play
        currentTrackIndex = 0;
        player.load(playlist[currentTrackIndex]);
    }

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            
            if (event.type == SDL_DROPFILE) {
                char* dropped_file = event.drop.file;
                std::string file_path = convertWindowsPathToWSL(dropped_file);
                SDL_free(dropped_file);
                try {
                    if (fs::is_directory(file_path)) {
                        for (const auto& entry : fs::directory_iterator(file_path)) {
                            if (entry.is_regular_file()) {
                                std::string ext = entry.path().extension().string();
                                if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg") {
                                    playlist.push_back(fs::absolute(entry.path()).string());
                                }
                            }
                        }
                    } else {
                        playlist.push_back(file_path);
                    }
                    std::sort(playlist.begin(), playlist.end());
                    playlist.erase(std::unique(playlist.begin(), playlist.end()), playlist.end());
                    savePlaylist(playlist);
                } catch (...) {
                    Logger::instance().log(LogLevel::ERROR, "Failed to process dropped file");
                }
            }
        }

        // Auto-advance or Loop
        if (player.isFinished()) {
            if (loop && currentTrackIndex >= 0) {
                // Replay same track
                if (player.load(playlist[currentTrackIndex])) {
                    player.setSpeed(speed);
                    player.setVolume(volume);
                    player.play();
                }
            } else if (currentTrackIndex >= 0 && currentTrackIndex + 1 < (int)playlist.size()) {
                // Next track
                currentTrackIndex++;
                if (player.load(playlist[currentTrackIndex])) {
                    player.setSpeed(speed);
                    player.setVolume(volume);
                    player.play();
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ---------------------------------------------------------
        // GUI Code
        // ---------------------------------------------------------
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("Music Player", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            // Header
            ImGui::TextDisabled("MUSIC PLAYER");
            ImGui::Separator();
            ImGui::Spacing();

            // Now Playing
            if (currentTrackIndex >= 0 && currentTrackIndex < (int)playlist.size()) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Now Playing: %s", playlist[currentTrackIndex].c_str());
            } else {
                ImGui::Text("No file loaded.");
            }
            ImGui::Spacing();

            // Controls
            if (ImGui::Button("<< Prev")) {
                if (currentTrackIndex > 0) {
                    currentTrackIndex--;
                    if (player.load(playlist[currentTrackIndex])) {
                        player.setSpeed(speed);
                        player.setVolume(volume);
                        player.play();
                    }
                }
            }
            ImGui::SameLine();
            
            if (player.isPlaying() && !player.isPaused()) {
                if (ImGui::Button("Pause", ImVec2(80, 0))) {
                    player.pause();
                }
            } else {
                if (ImGui::Button("Play", ImVec2(80, 0))) {
                    if (currentTrackIndex >= 0) {
                        if (player.isPaused()) {
                            player.resume();
                        } else {
                            if (player.load(playlist[currentTrackIndex])) {
                                player.setSpeed(speed);
                                player.setVolume(volume);
                                player.play();
                            }
                        }
                    } else if (!playlist.empty()) {
                        currentTrackIndex = 0;
                        if (player.load(playlist[currentTrackIndex])) {
                            player.setSpeed(speed);
                            player.setVolume(volume);
                            player.play();
                        }
                    }
                }
            }
            ImGui::SameLine();

            if (ImGui::Button("Stop")) {
                player.stop();
            }
            ImGui::SameLine();

            if (ImGui::Button("Next >>")) {
                if (currentTrackIndex + 1 < (int)playlist.size()) {
                    currentTrackIndex++;
                    if (player.load(playlist[currentTrackIndex])) {
                        player.setSpeed(speed);
                        player.setVolume(volume);
                        player.play();
                    }
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Speed Control
            ImGui::Text("Playback Speed:");
            ImGui::SameLine();
            if (ImGui::Button("0.75x")) { speed = 0.75f; player.setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::Button("1.0x")) { speed = 1.0f; player.setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::Button("1.5x")) { speed = 1.5f; player.setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::Button("2.0x")) { speed = 2.0f; player.setSpeed(speed); }
            ImGui::SameLine();
            ImGui::TextDisabled("(%.2fx)", speed);

            ImGui::Spacing();

            // Volume & Loop
            ImGui::Text("Volume:");
            ImGui::SameLine();
            if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 2.0f, "%.2f")) {
                player.setVolume(volume);
            }
            if (volume > 1.0f) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "BOOST ACTIVE");
            }

            ImGui::Checkbox("Loop Track", &loop);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Add Path UI
            static char pathBuffer[512] = "";
            ImGui::InputTextWithHint("##addpath", "Paste folder path here...", pathBuffer, IM_ARRAYSIZE(pathBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                std::string newPath = convertWindowsPathToWSL(pathBuffer);
                try {
                    if (!newPath.empty() && fs::exists(newPath) && fs::is_directory(newPath)) {
                        for (const auto& entry : fs::directory_iterator(newPath)) {
                            if (entry.is_regular_file()) {
                                std::string ext = entry.path().extension().string();
                                if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg") {
                                    playlist.push_back(fs::absolute(entry.path()).string());
                                }
                            }
                        }
                        std::sort(playlist.begin(), playlist.end());
                        playlist.erase(std::unique(playlist.begin(), playlist.end()), playlist.end());
                        savePlaylist(playlist);
                        memset(pathBuffer, 0, sizeof(pathBuffer));
                    }
                } catch (...) {}
            }
            ImGui::Spacing();

            // Clear Playlist Button
            if (ImGui::Button("Clear Playlist")) {
                player.stop();
                playlist.clear();
                currentTrackIndex = -1;
                savePlaylist(playlist);
            }
            ImGui::Spacing();

            // Playlist
            ImGui::Text("Playlist (%zu files)", playlist.size());
            ImGui::BeginChild("PlaylistRegion", ImVec2(0, -30), true);
            for (int i = 0; i < (int)playlist.size(); i++) {
                bool isSelected = (currentTrackIndex == i);
                std::string displayName = fs::path(playlist[i]).filename().string();
                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    currentTrackIndex = i;
                    if (player.load(playlist[currentTrackIndex])) {
                        player.setSpeed(speed);
                        player.setVolume(volume);
                        player.play();
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Quit App", ImVec2(100, 0))) {
                done = true;
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.00f); // Black background
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    player.stop();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
