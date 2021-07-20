#include "input_view.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <set>

#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <imgui.h>

InputView::InputView(float intersection_threshold, float segment_length, float stroke_thickness,
                     std::uint32_t background_color, std::uint32_t stroke_color)
    : m_intersection_threshold(intersection_threshold), m_stroke_segment_length(segment_length),
      m_stroke_thickness(stroke_thickness), m_background_color(background_color | 0xff << 24),
      m_stroke_color(stroke_color | 0xff << 24), m_stroke_history(1), m_history_position(0), m_drawing(false) {}

InputView::~InputView() {}

bool InputView::Stroke::Intersects(const InputView::Stroke& other, float threshold) const {
    if ((rect_max.x + threshold < other.rect_min.x || rect_max.y + threshold < other.rect_min.y) ||
        (other.rect_max.x + threshold < rect_min.x || other.rect_max.y + threshold < rect_min.y))
        return false;

    // FIXME: This is not smart at all.
    for (const auto& point_a : points) {
        for (const auto& point_b : other.points) {
            glm::vec2 delta = point_b - point_a;
            if (glm::dot(delta, delta) <= threshold * threshold)
                return true;
        }
    }
    return false;
}

void InputView::Show() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, m_background_color);
    bool visible = ImGui::BeginChild("Frame", ImVec2(0, 0), true, ImGuiWindowFlags_NoMove);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (!visible) {
        ImGui::EndChild();
        return;
    }

    bool dirty = false;
    bool history_changed = false;

    ImGui::InvisibleButton("Canvas", ImGui::GetContentRegionAvail(), ImGuiButtonFlags_MouseButtonLeft);
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        ImGui::OpenPopupOnItemClick("Menu");
    if (ImGui::BeginPopup("Menu")) {
        if (ImGui::MenuItem("Clear", nullptr, false, !m_glyph_strokes.empty())) {
            m_glyph_strokes.clear();
            history_changed = true;
            dirty = true;
        }
        if (ImGui::MenuItem("Undo", nullptr, false, m_history_position != 0)) {
            // TODO: Make history iterative and remember only the changes.
            --m_history_position;
            m_glyph_strokes = m_stroke_history[m_history_position];
            dirty = true;
        }
        if (ImGui::MenuItem("Redo", nullptr, false, m_history_position < m_stroke_history.size() - 1)) {
            ++m_history_position;
            m_glyph_strokes = m_stroke_history[m_history_position];
            dirty = true;
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Options")) {
            ImGui::PushItemWidth(ImGui::GetFontSize() * 8);

            dirty |= ImGui::InputFloat("Merging distance", &m_intersection_threshold, 1, 0, "%.1f");
            ImGui::InputFloat("Threshold", &m_stroke_segment_length, 1, 0, "%.1f");
            ImGui::InputFloat("Thickness", &m_stroke_thickness, 1, 0, "%.1f");

            ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(m_background_color);
            if (ImGui::ColorEdit3("Background", reinterpret_cast<float*>(&rgb)))
                m_background_color = ImGui::ColorConvertFloat4ToU32(rgb);

            rgb = ImGui::ColorConvertU32ToFloat4(m_stroke_color);
            if (ImGui::ColorEdit3("Stroke", reinterpret_cast<float*>(&rgb))) {
                m_stroke_color = ImGui::ColorConvertFloat4ToU32(rgb);
            }

            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    ImGuiIO& io = ImGui::GetIO();

    auto screen_offset = ImGui::GetCursorScreenPos();
    glm::vec2 canvas_position(io.MousePos.x - screen_offset.x, io.MousePos.y - screen_offset.y);

    // TODO: May be it will be a good idea to stop drawing the stroke after hitting the border.
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Set up a new stroke.
            m_glyph_strokes.emplace_back(canvas_position);
            printf("New stroke #%llu\n", m_glyph_strokes.size());
            m_drawing = true;
        } else if (ImGui::IsItemActive()) {
            auto& current_stroke_points = m_glyph_strokes.back().points;
            auto delta = canvas_position - current_stroke_points[current_stroke_points.size() - 2];
            if (glm::dot(delta, delta) <= m_stroke_segment_length * m_stroke_segment_length) {
                current_stroke_points.back() = canvas_position;
            } else {
                current_stroke_points.push_back(canvas_position);
            }
            ImGui::SetTooltip("#%llu:%llu (%.1f, %.1f)", m_glyph_strokes.size(), current_stroke_points.size(),
                              canvas_position.x, canvas_position.y);
        } else if (m_drawing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            // Calculate the bounding box for the completed stroke.
            auto& last_stroke = m_glyph_strokes.back();
            for (const auto& point : last_stroke.points) {
                last_stroke.rect_min = glm::min(point, last_stroke.rect_min);
                last_stroke.rect_max = glm::max(point, last_stroke.rect_max);
            }

            dirty = true;
            history_changed = true;
            m_drawing = false;
        }
    }

    if (history_changed) {
        ++m_history_position;
        m_stroke_history.resize(m_history_position);
        m_stroke_history.push_back(m_glyph_strokes);
        printf("History resized to %llu\n", m_stroke_history.size());
    }

    if (dirty) {
        printf("Process %llu strokes\n", m_glyph_strokes.size());
        m_glyphs.clear();

        std::vector<std::set<size_t>> intersection_groups;

        // FIXME: Allow one stroke to merge several others that are intersecting with it into a single glyph.
        //        Currently it's pretty much broken. :^(
        for (size_t stroke_index = 0; stroke_index != m_glyph_strokes.size(); ++stroke_index) {
            const auto& stroke_a = m_glyph_strokes[stroke_index];

            auto group_iterator = std::find_if(intersection_groups.begin(), intersection_groups.end(),
                                               [&stroke_index](const std::set<size_t>& group) -> bool {
                                                   return group.find(stroke_index) != group.end();
                                               });
            if (group_iterator == intersection_groups.end()) {
                intersection_groups.emplace_back(std::initializer_list<size_t>{stroke_index});
                group_iterator = intersection_groups.end() - 1;
            }

            for (size_t other_index = stroke_index + 1; other_index != m_glyph_strokes.size(); ++other_index) {
                const auto& stroke_b = m_glyph_strokes[other_index];
                if (stroke_a.Intersects(stroke_b, m_intersection_threshold)) {
                    group_iterator->insert(other_index);
                }
            }
        }

        for (const auto& group : intersection_groups) {
            glm::vec2 rect_min = m_glyph_strokes[*group.begin()].rect_min;
            glm::vec2 rect_max = m_glyph_strokes[*group.begin()].rect_max;
            std::vector<std::reference_wrapper<const Stroke>> strokes;
            strokes.reserve(group.size());
            for (auto index : group) {
                const auto& stroke = m_glyph_strokes[index];
                strokes.push_back(stroke);
                rect_min = glm::min(rect_min, stroke.rect_min);
                rect_max = glm::max(rect_max, stroke.rect_max);
            }
            m_glyphs.emplace_back(rect_min, rect_max, strokes);
        }
    }

    auto* draw_list = ImGui::GetWindowDrawList();
    for (const auto& stroke : m_glyph_strokes) {
        const auto& points = stroke.points;
        assert(points.size() >= 2);

        std::vector<ImVec2> stroke_points;
        stroke_points.reserve(points.size());
        for (const auto& point : points)
            stroke_points.emplace_back(point.x + screen_offset.x, point.y + screen_offset.y);
        draw_list->AddPolyline(stroke_points.data(), stroke_points.size(), m_stroke_color, ImDrawFlags_RoundCornersAll,
                               m_stroke_thickness);
    }
    for (const auto& glyph : m_glyphs) {
        draw_list->AddRect(ImVec2(glyph.rect_min.x + screen_offset.x, glyph.rect_min.y + screen_offset.y),
                           ImVec2(glyph.rect_max.x + screen_offset.x, glyph.rect_max.y + screen_offset.y),
                           m_background_color ^ 0xffffff);
    }

    ImGui::EndChild();
}

void InputView::QueryGlyphBuffer(size_t index, unsigned buffer_width, unsigned buffer_height,
                                 std::vector<float>& output_destination) const {
    assert(index < m_glyphs.size());
    assert(output_destination.size() >= static_cast<size_t>(buffer_width) * static_cast<size_t>(buffer_height));

    const auto& glyph = m_glyphs[index];

    // TODO: Initialize GL-related stuff in the constructor and keep it.

    GLuint buffer_descriptor = 0;
    glGenFramebuffers(1, &buffer_descriptor);
    assert(buffer_descriptor != 0);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer_descriptor);

    GLuint buffer_texture_descriptor = 0;
    glGenTextures(1, &buffer_texture_descriptor);
    assert(buffer_texture_descriptor != 0);
    glBindTexture(GL_TEXTURE_2D, buffer_texture_descriptor);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, buffer_width, buffer_height, 0, GL_RED, GL_FLOAT, nullptr);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, buffer_texture_descriptor, 0);

    GLenum target_buffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(sizeof(target_buffers) / sizeof(*target_buffers), target_buffers);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    GLint success;

    static const char* vertex_shader_text = "#version 330 core\n"
                                            "layout (location = 0) in vec2 pos;\n"
                                            "uniform vec4 rect;\n"
                                            "void main() {\n"
                                            "    gl_Position = vec4((pos - rect.xy) / rect.zw * 2 - 1, 0.0, 1.0);\n"
                                            "}";
    GLuint vertex_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    assert(vertex_shader_handle != 0);
    glShaderSource(vertex_shader_handle, 1, &vertex_shader_text, nullptr);
    glCompileShader(vertex_shader_handle);

    glGetShaderiv(vertex_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char error_message[512];
        glGetShaderInfoLog(vertex_shader_handle, sizeof(error_message), nullptr, error_message);
        printf("Failed to compile vertex shader:\n%s\n", error_message);
        assert(false);
    }

    static const char* geometry_shader_text = "#version 330 core\n"
                                              "layout (lines) in;\n"
                                              "layout (triangle_strip, max_vertices = 16) out;\n"
                                              "out float col;\n"
                                              "uniform float thickness;\n"
                                              "void main() {\n"
                                              "    vec2 dir = gl_in[1].gl_Position.xy - gl_in[0].gl_Position.xy;\n"
                                              "    float l = pow(dir.x * dir.x + dir.y * dir.y, 0.5);\n"
                                              "    dir /= l;\n"
                                              "    vec2 sideways = vec2(dir.y, -dir.x) * thickness;\n"
                                              "    dir *= thickness;\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy -= sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy -= sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    col = 1.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    col = 1.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy += sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy += sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    EndPrimitive();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy -= sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy -= sideways * 0.5;\n"
                                              "    gl_Position.xy -= dir * 0.7;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    col = 1.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy += sideways * 0.5;\n"
                                              "    gl_Position.xy -= dir * 0.7;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[0].gl_Position;\n"
                                              "    gl_Position.xy += sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    EndPrimitive();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy -= sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy -= sideways * 0.5;\n"
                                              "    gl_Position.xy += dir * 0.7;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    col = 1.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy += sideways * 0.5;\n"
                                              "    gl_Position.xy += dir * 0.7;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    gl_Position = gl_in[1].gl_Position;\n"
                                              "    gl_Position.xy += sideways;\n"
                                              "    col = 0.0;\n"
                                              "    EmitVertex();\n"
                                              "    EndPrimitive();\n"
                                              "}";
    GLuint geometry_shader_handle = glCreateShader(GL_GEOMETRY_SHADER);
    assert(geometry_shader_handle != 0);
    glShaderSource(geometry_shader_handle, 1, &geometry_shader_text, nullptr);
    glCompileShader(geometry_shader_handle);

    glGetShaderiv(geometry_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char error_message[512];
        glGetShaderInfoLog(geometry_shader_handle, sizeof(error_message), nullptr, error_message);
        printf("Failed to compile geometry shader:\n%s\n", error_message);
        assert(false);
    }

    static const char* fragment_shader_text = "#version 330 core\n"
                                              "in float col;\n"
                                              "layout (location = 0) out float color;\n"
                                              "void main() {\n"
                                              "    color = col;\n"
                                              "}";
    GLuint fragment_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
    assert(fragment_shader_handle != 0);
    glShaderSource(fragment_shader_handle, 1, &fragment_shader_text, nullptr);
    glCompileShader(fragment_shader_handle);

    glGetShaderiv(fragment_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char error_message[512];
        glGetShaderInfoLog(fragment_shader_handle, sizeof(error_message), nullptr, error_message);
        printf("Failed to compile fragment shader:\n%s\n", error_message);
        assert(false);
    }

    GLuint shader_program_id = glCreateProgram();
    glAttachShader(shader_program_id, vertex_shader_handle);
    glAttachShader(shader_program_id, geometry_shader_handle);
    glAttachShader(shader_program_id, fragment_shader_handle);
    glLinkProgram(shader_program_id);

    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &success);
    if (!success) {
        char error_message[512];
        glGetProgramInfoLog(shader_program_id, sizeof(error_message), nullptr, error_message);
        printf("Failed to link shader program:\n%s\n", error_message);
        assert(false);
    }

    glDeleteShader(vertex_shader_handle);
    glDeleteShader(fragment_shader_handle);

    glUseProgram(shader_program_id);

    glm::vec2 rect_min = glyph.rect_min;
    glm::vec2 rect_size = glyph.rect_max - rect_min;
    float half_dimensions_difference = (rect_size.x - rect_size.y) * 0.5f;
    if (half_dimensions_difference > 0) {
        rect_min.y -= half_dimensions_difference;
        rect_size.y = rect_size.x;
    } else {
        rect_min.x += half_dimensions_difference;
        rect_size.x = rect_size.y;
    }

    rect_size.x /= buffer_width;
    rect_size.y /= buffer_height;
    rect_min -= rect_size;
    rect_size.x *= buffer_width + 2;
    rect_size.y *= buffer_height + 2;

    glViewport(0, 0, buffer_width, buffer_height);
    glUniform4f(glGetUniformLocation(shader_program_id, "rect"), rect_min.x, rect_min.y, rect_size.x, rect_size.y);
    glUniform1f(glGetUniformLocation(shader_program_id, "thickness"), 2.0f / buffer_width);

    glClearColor(0, 0, 0, 0);
    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX);
    glDepthMask(GL_FALSE);

    glClear(GL_COLOR_BUFFER_BIT);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    assert(vbo != 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    assert(vao != 0);
    glBindVertexArray(vao);

    const GLuint vertex_attrib_index = 0;
    glEnableVertexAttribArray(vertex_attrib_index);
    glVertexAttribPointer(vertex_attrib_index, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    for (auto stroke : glyph.strokes) {
        const auto& points = stroke.get().points;
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(*points.data()), points.data(), GL_STATIC_DRAW);
        glDrawArrays(GL_LINE_STRIP, 0, points.size());
    }
    glDisableVertexAttribArray(vertex_attrib_index);

    glReadPixels(0, 0, buffer_width, buffer_height, GL_RED, GL_FLOAT, output_destination.data());

    glDeleteVertexArrays(1, &vao);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);

    glUseProgram(0);
    glDeleteProgram(shader_program_id);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &buffer_texture_descriptor);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &buffer_descriptor);
}
