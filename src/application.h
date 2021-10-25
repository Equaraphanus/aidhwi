#pragma once

#include <memory>

#include <SDL.h>

#include "event.h"
#include "input_view.h"
#include "network_editor.h"
#include <neural/network.h>

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();

    Event<int, int>& Resized() { return m_resized; };

private:
    SDL_Window* m_window;
    SDL_GLContext m_context;
    bool m_running;
    std::unique_ptr<Neural::Network> m_network;
    std::unique_ptr<InputView> m_input_view;
    std::unique_ptr<NetworkEditor> m_network_editor;
    unsigned m_glyph_buffer_width;
    unsigned m_glyph_buffer_height;
    std::vector<std::string> m_output_options;
    int m_selected_option;

    Event<int, int> m_resized;

    void Render();
    void HandleEvent(SDL_Event&);
};
