#pragma once

#include <functional>
#include <string>
#include <vector>

#include <glm/ext/vector_float2.hpp>
#include <neural/network.h>

class InputView {
public:
    InputView(const Neural::Network&, unsigned, unsigned, const std::vector<std::string>&);
    ~InputView();

    void Show();

    void QueryCurrentGlyphBuffer(std::vector<double>&) const;

    const std::string& GetCurrentInput() const;

    inline void SetNetwork(const Neural::Network& new_network) { m_network = new_network; }

    inline void SetOutputOptions(const std::vector<std::string>& new_options) { m_output_options = new_options; }
    inline const std::vector<std::string>& GetOutputOptions() const { return m_output_options; }

    void ResizeBuffer(unsigned new_width, unsigned new_height);

    inline unsigned GetBufferWidth() const { return m_buffer_width; }
    inline unsigned GetBufferHeight() const { return m_buffer_height; }

private:
    std::reference_wrapper<const Neural::Network> m_network;
    unsigned m_buffer_width;
    unsigned m_buffer_height;
    std::vector<std::string> m_output_options;
    std::vector<std::vector<glm::vec2>> m_glyph_strokes;
    std::vector<std::vector<std::vector<glm::vec2>>> m_stroke_history;
    size_t m_history_position;
    float m_stroke_segment_length;
    float m_stroke_thickness;
    std::uint32_t m_background_color;
    std::uint32_t m_stroke_color;
};
