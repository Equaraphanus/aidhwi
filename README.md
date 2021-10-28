# Aidhwi

**Aidhwi** - *AI* *D*riven *H*and *W*riting *I*nput.

## Purpose
An attempt to recreate a handwriting recognition input method from scratch in C++.

## Components
- Front-end
- Character recognition neural network
- TODO

## Dependencies
Front-end:
- [SDL2](https://libsdl.org/) &mdash; Input and display abstraction
- [GLAD](https://glad.dav1d.de/) &mdash; GL loader
- [Dear ImGui](https://github.com/ocornut/imgui) &mdash; Immediate mode GUI library
- [GLM](https://github.com/g-truc/glm) &mdash; GL math

## Building
Clone the repository and follow the instructions for your platform:

**Note:** This repository has several dependencies as [Git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).
They should be initialized by running `git submodule init && git submodule update` before proceeding.
Alternatively you can pass `--recurse-submodules` to `git clone` command when cloning the repo.

**Note:** This project uses [CMake](https://cmake.org) to generate build scripts.
Make sure you have CMake version 3.13 or higher.

**Note:** The following instructions assume that you are using [Ninja](https://ninja-build.org/) as a build system.
Get Ninja if you want to follow the instructions exactly.

### Desktop
1. Ensure you have all required dependencies installed and configured (specifically SDL).
2. Create a build directory (preferably somewhere outside of the project structure) and `cd` to it.
3. Run `cmake -G Ninja "/path/to/this/repo" -B .`.
4. Run `ninja` to build the generated project.

To test the build copy the `res` directory (and all required DLLs if on Windows) to your build and run `./aidhwi`.

### Web
1. Get Emscripten and its dependencies: [emscripten.org](https://emscripten.org)
2. Optional: go through the [tutorial](https://emscripten.org/docs/getting_started/Tutorial.html) to check if it is set up properly
3. Create a build directory (preferably somewhere outside of the project structure) and `cd` to it.
4. Run `cmake -G Ninja "/path/to/this/repo" "-DCMAKE_TOOLCHAIN_FILE=/path/to/Emscripten.cmake" -B .`.\
   **Note:** `Emscripten.cmake` can usually be found in `/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/`.
5. Run `ninja` to build the generated project.

To test the build start any http server in the build directory to serve on port `8000` (or any other available one) and navigate to `http://localhost:8000/aidhwi.html`.
