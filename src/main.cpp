#include <iostream>

#include "application.h"

int main(int argc, char** argv) {
    std::cerr << "Executable: \"" << argv[0] << "\"; argc=" << argc << std::endl;
    Application app;
    if (!app.Init()) {
        std::cerr << "Error: Initialization failed" << std::endl;
        return 1;
    }
    std::cerr << "Initialized successfully" << std::endl;

    app.Resized() += [](int new_width, int new_height) {
        std::cerr << "Resized to " << new_width << "x" << new_height << std::endl;
    };

    app.Run();

    return 0;
}
