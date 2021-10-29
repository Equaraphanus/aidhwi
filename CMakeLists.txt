cmake_minimum_required(VERSION 3.13)

project(aidhwi VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(set_flags target)
    target_compile_features(${target}
        PUBLIC
        cxx_std_17
    )
    set_target_properties(${target}
        PROPERTIES
        CXX_EXTENSIONS OFF
    )

    if(MSVC)
        target_compile_options(${target}
            PRIVATE
            /W4
            /WX
        )
    else()
        target_compile_options(${target}
            PRIVATE
            -Wall
            -Wextra
            -pedantic
            -Wshadow
            -Wpointer-arith
        )
    endif()

    if(EMSCRIPTEN)
        target_compile_options(${target}
            PUBLIC
            "SHELL:-s USE_SDL=2"
        )
        target_link_options(${target}
            PUBLIC
            "SHELL:-s USE_SDL=2"
            "SHELL:-s INITIAL_MEMORY=33554432"
            "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/platform/web/shell.html"
            "SHELL:--preload-file res"
        )
    endif()
endfunction()

set(CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake_modules"
    ${CMAKE_MODULE_PATH}
)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gsource-map")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gsource-map")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -gsource-map")
else()
    find_package(SDL2 REQUIRED)
endif()


add_subdirectory(deps)

include_directories(
    ${SDL2_INCLUDE_DIR}
    deps/glad/include
    deps/imgui
    src
)

add_library(neural
    src/neural/network.cpp
)
set_flags(neural)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/application.cpp
    src/inspector.cpp
    src/network_editor.cpp
    src/input_view.cpp
    deps/imgui/backends/imgui_impl_sdl.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARY}
    glm
    imgui
    neural
)

if(EMSCRIPTEN)
    set(NO_GEOMETRY_SHADERS ON)

    target_link_libraries(${PROJECT_NAME}
        idbfs.js
    )
else()
    option(NO_GEOMETRY_SHADERS "Do not use geometry shaders" OFF)

    target_link_libraries(${PROJECT_NAME}
        glad
        ${CMAKE_DL_LIBS}
    )
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD
    )
endif()

if (NO_GEOMETRY_SHADERS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        NO_GEOMETRY_SHADERS
    )
endif()

set_flags(${PROJECT_NAME})

