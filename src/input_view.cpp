#include "input_view.h"

#include <glm/geometric.hpp>
#include <imgui.h>

InputView::InputView(const Neural::Network& ann, unsigned buffer_width, unsigned buffer_height,
                     const std::vector<std::string>& output_options)
    : m_network(ann), m_buffer_width(buffer_width), m_buffer_height(buffer_height), m_output_options(output_options),
      m_stroke_history(1), m_history_position(0), m_stroke_segment_length(8.0f), m_stroke_thickness(4.0f),
      m_background_color(0xff2c451a), m_stroke_color(0xffffffff) {}

InputView::~InputView() {}

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
            m_glyph_strokes.emplace_back(2, canvas_position);
            printf("New stroke #%llu\n", m_glyph_strokes.size());
        } else if (ImGui::IsItemActive()) {
            auto& current_stroke = m_glyph_strokes.back();
            auto delta = canvas_position - current_stroke[current_stroke.size() - 2];
            if (glm::dot(delta, delta) <= m_stroke_segment_length * m_stroke_segment_length) {
                current_stroke.back() = canvas_position;
            } else {
                m_glyph_strokes.back().push_back(canvas_position);
            }
            ImGui::SetTooltip("#%llu:%llu (%.1f, %.1f)", m_glyph_strokes.size(), m_glyph_strokes.back().size(),
                              canvas_position.x, canvas_position.y);
        } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dirty = true;
            history_changed = true;
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
    }

    auto* draw_list = ImGui::GetWindowDrawList();
    for (const auto& stroke : m_glyph_strokes) {
        assert(stroke.size() >= 2);

        std::vector<ImVec2> stroke_points;
        stroke_points.reserve(stroke.size());
        for (const auto& point : stroke)
            stroke_points.emplace_back(point.x + screen_offset.x, point.y + screen_offset.y);
        draw_list->AddPolyline(stroke_points.data(), stroke_points.size(), m_stroke_color, ImDrawFlags_RoundCornersAll, m_stroke_thickness);
    }

    ImGui::EndChild();
}

void InputView::QueryCurrentGlyphBuffer(std::vector<double>& output_destination) const {
    // TODO
    (void)output_destination;
}

void InputView::ResizeBuffer(unsigned new_width, unsigned new_height) {
    m_buffer_width = new_width;
    m_buffer_height = new_height;
}
