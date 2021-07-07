add_subdirectory(glad)

add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
)

add_subdirectory(glm)

if(WIN32)
    target_link_libraries(imgui
        imm32 dwmapi
    )
endif()
