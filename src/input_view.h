#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <glm/ext/vector_float2.hpp>
#include <neural/network.h>

class InputView {
public:
    InputView(float = 16, float = 8, float = 4, std::uint32_t = 0x2c451a, std::uint32_t = 0xffffff);
    ~InputView();

    void Show();

    inline size_t GetGlyphCount() const { return m_glyphs.size(); }

    void QueryGlyphBuffer(size_t, unsigned, unsigned, std::vector<float>&) const;

private:
    struct Stroke {
        glm::vec2 rect_min;
        glm::vec2 rect_max;
        std::vector<glm::vec2> points;

        Stroke(const glm::vec2& first_point) : rect_min(first_point), rect_max(first_point), points(2, first_point) {}

        bool Intersects(const Stroke&, float) const;
    };

    struct Glyph {
        glm::vec2 rect_min;
        glm::vec2 rect_max;
        std::vector<std::reference_wrapper<const Stroke>> strokes;

        Glyph(const Stroke& stroke) : rect_min(stroke.rect_min), rect_max(stroke.rect_max), strokes{stroke} {}
        Glyph(const glm::vec2& min, const glm::vec2& max,
              const std::vector<std::reference_wrapper<const Stroke>>& stroke_refs)
            : rect_min(min), rect_max(max), strokes(stroke_refs) {}
    };

    float m_intersection_threshold;
    float m_stroke_segment_length;
    float m_stroke_thickness;
    std::uint32_t m_background_color;
    std::uint32_t m_stroke_color;

    std::vector<Stroke> m_glyph_strokes;
    std::vector<Glyph> m_glyphs;
    std::vector<std::vector<Stroke>> m_stroke_history;
    size_t m_history_position;
    bool m_drawing;
};
