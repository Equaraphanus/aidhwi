#pragma once

#include <memory>

#include <SDL.h>

#include "event.h"
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
    std::vector<double> m_network_inputs;

    Event<int, int> m_resized;

    void Render();
    void HandleEvent(SDL_Event&);
};
