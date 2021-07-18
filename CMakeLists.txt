cmake_minimum_required(VERSION 3.6)

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
endfunction()

set(CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake_modules"
	${CMAKE_MODULE_PATH}
)

find_package(SDL2 REQUIRED)

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
    deps/imgui/backends/imgui_impl_sdl.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARY}
    glad
    glm
    imgui
    neural
)

set_flags(${PROJECT_NAME})

