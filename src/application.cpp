#include "application.h"

#include <iostream>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>
#include <glad/glad.h>
#include <imgui.h>

#include "inspector.h"
#include "neural/network.h"

Application::Application() : m_running(false) {}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(m_context);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Application::Init() {
    // Init SDL
    std::cerr << "Initializing SDL..." << std::endl;
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "Error: Failed to initialize SDL" << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int width = 960;
    int height = 720;
    std::cerr << "Creating window..." << std::endl;
    m_window = SDL_CreateWindow("Aidhwi demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (m_window == NULL) {
        std::cerr << "Error: Failed to create window" << std::endl;
        return false;
    }

    std::cerr << "Creating OpenGL context..." << std::endl;
    m_context = SDL_GL_CreateContext(m_window);
    if (m_context == NULL) {
        std::cerr << "Error: Failed to create OpenGL context" << std::endl;
        return false;
    }

    // V-Sync
    SDL_GL_SetSwapInterval(1);

    std::cerr << "Initializing OpenGL..." << std::endl;
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Error: Failed to initialize OpenGL" << std::endl;
        return false;
    }

    glClearColor(0.25f, 0.5f, 0.5f, 1.0f);

    // Init ImGUI
    std::cerr << "Initializing ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    std::cerr << "Loading fonts..." << std::endl;
    io.Fonts->AddFontFromFileTTF("res/fonts/NotoSans-Regular.ttf", 18, NULL, io.Fonts->GetGlyphRangesDefault());
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("res/fonts/NotoSans-Regular.ttf", 18, &config, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromFileTTF("res/fonts/NotoSansJP-Regular.otf", 18, &config, io.Fonts->GetGlyphRangesJapanese());
    io.Fonts->Build();

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.34f, 0.98f, 0.26f, 1.00f);

    m_network = std::make_unique<Neural::Network>(3, std::vector<size_t>{5, 1});
    m_network->Randomize(1337);
    std::cerr << m_network->ComputeOutput({0.25, 0.5, 0.75})[0] << std::endl;

    return true;
}

void Application::Run() {
    m_running = true;
    while (m_running) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            HandleEvent(evt);
            if (evt.type == SDL_QUIT)
                m_running = false;
        }
        Render();
    }
}

void Application::HandleEvent(SDL_Event& evt) {
    if (ImGui_ImplSDL2_ProcessEvent(&evt))
        return;

    switch (evt.type) {
    case SDL_KEYDOWN:
        break;
    case SDL_KEYUP:
        break;
    case SDL_TEXTEDITING:
        break;
    case SDL_WINDOWEVENT:
        switch (evt.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            Resized().Invoke(evt.window.data1, evt.window.data2);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void Application::Render() {
    static unsigned last_timestamp = SDL_GetTicks();
    unsigned timestamp = SDL_GetTicks();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();

    // ImGUI
    ImGui::ShowDemoWindow();

    ImGui::Begin("Inspector");
    if (ImGui::CollapsingHeader("Neural network")) {
        Inspector::ShowProperty(*m_network);
        if (ImGui::Button("Randomize")) {
            m_network->Randomize();
        }
    }
    ImGui::End();

    ImGui::Render();
    SDL_GL_MakeCurrent(m_window, m_context);
    glClearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(m_window);

    last_timestamp = timestamp;
}
