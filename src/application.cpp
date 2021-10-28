#include "application.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>
#include <imgui.h>

#include "inspector.h"
#include "neural/network.h"
#include "util/literals.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/glad.h>
#endif

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
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL" << std::endl;
        std::cerr << SDL_GetError() << std::endl;
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

#ifndef __EMSCRIPTEN__
    std::cerr << "Initializing OpenGL..." << std::endl;
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Error: Failed to initialize OpenGL" << std::endl;
        return false;
    }
#endif
    std::cerr << glGetString(GL_VERSION) << std::endl;
    std::cerr << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glClearColor(0.25f, 0.5f, 0.5f, 1.0f);

    // Init ImGUI
    std::cerr << "Initializing ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init("#version 100");

    auto& io = ImGui::GetIO();

#ifdef __EMSCRIPTEN__
    io.IniFilename = "config/imgui.ini";
#endif

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

    std::cerr << "Setting up input view..." << std::endl;
    m_glyph_buffer_width = 16;
    m_glyph_buffer_height = 16;
    m_output_options.assign({"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"});
    // Arbitrarily chosen number, probably should be tuned by trial and error.
    const size_t hidden_layer_size = m_output_options.size() * 2;

    m_network = std::make_unique<Neural::Network>(m_glyph_buffer_width * m_glyph_buffer_height,
                                                  std::vector<size_t>{hidden_layer_size, m_output_options.size()});
    m_network->Randomize(1337);

    m_input_view = std::make_unique<InputView>();

    m_network_editor = std::make_unique<NetworkEditor>(*m_network);

    return true;
}

void Application::Run() {
#ifdef __EMSCRIPTEN__
    auto loop_callback = [](void* arg) -> void {
        Application& app = *static_cast<Application*>(arg);
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            app.HandleEvent(evt);
        }
        app.Render();
    };
    emscripten_set_main_loop_arg(loop_callback, this, 0, true);
#else
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
#endif
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
    // TODO: Move to class declaration.
    // static unsigned last_timestamp = SDL_GetTicks();
    // unsigned timestamp = SDL_GetTicks();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();

    // ImGUI
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::ShowDemoWindow();

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Neural network"))
        m_network_editor->Show();
    ImGui::End();

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Input demo");
    bool glyph_changed = m_input_view->Show(ImVec2(ImGui::GetContentRegionAvailWidth() - ImGui::GetFontSize() * 12, 0));
    size_t glyph_count = m_input_view->GetGlyphCount();
    if (glyph_changed && glyph_count != 0) {
        std::vector<float> buffer(m_network_editor->GetInputs().size(), 0);
        m_input_view->QueryGlyphBuffer(glyph_count - 1, m_glyph_buffer_width, m_glyph_buffer_height, buffer);
        std::vector<double> inputs(buffer.begin(), buffer.end());
        auto outputs = m_network->ComputeOutput(inputs);
        m_selected_option = 0;
        double certainty = 0;
        for (size_t i = 0; i != outputs.size(); ++i) {
            if (outputs[i] > certainty) {
                certainty = outputs[i];
                m_selected_option = i;
            }
        }
    }
    ImGui::SameLine();
    ImGui::BeginChild("Tools");
    bool wants_feed_to_ann = ImGui::Button("Feed the last glyph to ANN") && glyph_count != 0;
    // TODO: Show radio buttons for every possible value.
    bool wants_add_as_record = ImGui::Button("Add as an example record") && glyph_count != 0;
    for (size_t option_index = 0; option_index != m_output_options.size(); ++option_index) {
        ImGui::RadioButton(m_output_options[option_index].c_str(), &m_selected_option, option_index);
    }
    if (wants_feed_to_ann || wants_add_as_record) {
        std::vector<float> buffer(m_network_editor->GetInputs().size(), 0);
        m_input_view->QueryGlyphBuffer(glyph_count - 1, m_glyph_buffer_width, m_glyph_buffer_height, buffer);

        for (size_t y = 0; y != m_glyph_buffer_height; ++y) {
            for (size_t x = 0; x != m_glyph_buffer_width; ++x) {
                // static constexpr const char* brightness_values[] = {"  ", ".'", "~~", "][", "XX", "WM", "$$"};
                // static constexpr const char* brightness_values[] = {"  ", "\xb0\xb0", "\xb1\xb1", "\xb2\xb2", "\xdb\xdb"};
                static constexpr const char* brightness_values[] = {"  ", "`,", "::", "[]", "WM"};
                static constexpr size_t brightness_levels = sizeof(brightness_values) / sizeof(*brightness_values);
                std::cout << brightness_values[std::clamp(
                    static_cast<size_t>(buffer[y * m_glyph_buffer_width + x] * brightness_levels), 0_zu,
                    brightness_levels - 1)];
            }
            std::cout << std::endl;
        }

        if (wants_feed_to_ann)
            m_network_editor->SetInputs(buffer);
        if (wants_add_as_record) {
            auto target_outputs = std::vector<double>(10);
            target_outputs[m_selected_option] = 1.0;
            m_network_editor->AddLearningExampleRecord(buffer, target_outputs);
        }
    }
    ImGui::EndChild();
    ImGui::End();

    ImGui::Render();
    SDL_GL_MakeCurrent(m_window, m_context);
    glClearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (glyph_count != 0) {
        glViewport(0, 0, 720, 720);
        m_input_view->DrawGlyphBuffer(glyph_count - 1);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(m_window);

    // last_timestamp = timestamp;
}
