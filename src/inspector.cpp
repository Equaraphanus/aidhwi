#include "inspector.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "neural/network.h"

namespace Inspector {

static bool RoundButton(const std::string& id, float size = 32) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, size / 2);
    bool result = ImGui::Button(id.c_str(), ImVec2(size, size));
    ImGui::PopStyleVar();
    return result;
}

static bool NeuronWidget(const std::string& id, double bias, float size = 32) {
    float hue = 0.17 * (std::tanh(bias) + 1);
    ImVec4 color;
    color.w = 1;

    ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 0.8f, color.x, color.y, color.z);
    ImGui::PushStyleColor(ImGuiCol_Button, color);

    ImGui::ColorConvertHSVtoRGB(hue, 0.8f, 1.0f, color.x, color.y, color.z);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

    ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, color.x, color.y, color.z);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));

    bool result = RoundButton(id, size);

    ImGui::PopStyleColor(4);

    return result;
}

static void NeuronTooltip(double bias, const std::vector<double>& weights, std::optional<double> output = {}) {
    ImGui::BeginTooltip();
    ImGui::BeginTable("Info", 2, ImGuiTableFlags_RowBg);

    for (size_t input_index = 0; input_index != weights.size(); ++input_index) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Weight %zu", input_index);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%f", weights[input_index]);
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Bias");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%f", bias);

    if (output.has_value()) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Output");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%f", output.value());
    }

    ImGui::EndTable();
    ImGui::EndTooltip();
}

// FIXME: The implementation is currently quite hackish and definitely needs to be reworked at some point.
static void DrawNetworkConnections(const Neural::Network& ann, std::vector<double>* inputs,
                                   std::vector<double>* outputs) {
    const auto& weights = ann.GetWeights();
    const auto& biases = ann.GetBiases();

    assert(!inputs || inputs->size() == weights.front().front().size());
    assert(!outputs || outputs->size() == weights.back().size());

    const float circle_size = 32;
    const size_t max_layer_size = ann.GetMaxLayerSize();
    const float offset = circle_size + ImGui::GetStyle().ItemSpacing.y;

    char name_buffer[32];

    std::optional<std::vector<double>> input_buffer;
    std::optional<std::vector<double>> output_buffer;

    if (inputs) {
        input_buffer.emplace(max_layer_size);
        std::copy(inputs->begin(), inputs->end(), input_buffer->begin());
        output_buffer.emplace(max_layer_size);
    }

    ImGui::BeginGroup();
    const size_t inputs_count = weights.front().front().size();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset * (max_layer_size - inputs_count) * 0.5f);
    for (size_t input_index = 0; input_index != inputs_count; ++input_index) {
        if (inputs) {
            snprintf(name_buffer, sizeof(name_buffer), "Input %zu", input_index);
            // Space input fields evenly with the same offset as for neurons.
            // This will hopefully become unnecessary as soon as the manual positioning will be implemented.
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                                ImVec2(ImGui::GetStyle().ItemSpacing.x,
                                       ImGui::GetStyle().ItemSpacing.y + (circle_size - ImGui::GetFontSize()) * 0.5f));
            ImGui::PushItemWidth(100);
            ImGui::InputDouble(name_buffer, &inputs->at(input_index), 0.0, 0.0, "%.3f");
            ImGui::PopItemWidth();
            ImGui::PopStyleVar();
        } else {
            snprintf(name_buffer, sizeof(name_buffer), "%zu", input_index);
            RoundButton(name_buffer, circle_size);
        }
    }
    ImGui::EndGroup();

    for (size_t layer_index = 0; layer_index != weights.size(); ++layer_index) {
        const auto& layer_weights = weights[layer_index];
        const auto& layer_biases = biases[layer_index];

        if (input_buffer.has_value()) {
            ann.ComputeOutputForLayer(layer_index, input_buffer.value(), output_buffer.value());
            input_buffer->swap(output_buffer.value());
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset * (max_layer_size - layer_weights.size()) * 0.5f);
        std::optional<double> neuron_output;
        for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
            if (input_buffer.has_value()) {
                neuron_output = input_buffer->at(neuron_index);
                if (outputs)
                    outputs->at(neuron_index) = *neuron_output;
            }
            snprintf(name_buffer, sizeof(name_buffer), "%zu", neuron_index);
            NeuronWidget(name_buffer, layer_biases[neuron_index], circle_size);
            if (ImGui::IsItemHovered())
                NeuronTooltip(layer_biases[neuron_index], layer_weights[neuron_index], neuron_output);
        }
        ImGui::EndGroup();
    }
}

static void DrawNetworkLayers(const Neural::Network& ann) {
    const auto& weights = ann.GetWeights();
    const auto& biases = ann.GetBiases();

    for (size_t layer_index = 0; layer_index != weights.size(); ++layer_index) {
        if (!ImGui::TreeNode((void*)(intptr_t)layer_index, "Layer %zu", layer_index))
            continue;

        const auto& layer_weights = weights[layer_index];
        const auto& layer_biases = biases[layer_index];
        char name_buffer[48];

        const float column_width = ImGui::GetFontSize() * 5;

        ImGui::SetNextWindowContentSize(ImVec2(column_width * (layer_weights.front().size() + 2), FLT_MIN));
        if (!ImGui::BeginChild(
                "Container",
                ImVec2(ImGui::GetContentRegionAvail().x,
                       (ImGui::GetFontSize() + ImGui::GetStyle().CellPadding.y * 2) * (layer_weights.size() + 1) +
                           ImGui::GetStyle().ScrollbarSize),
                false, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGui::EndChild();
            ImGui::TreePop();
            continue;
        }

        ImVec4 color;
        color.w = 1;

        ImGui::BeginTable("Biases", 2,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoHostExtendX,
                          ImVec2(column_width * 2, 0));
        ImGui::TableSetupColumn("Neuron");
        ImGui::TableSetupColumn("Bias");
        ImGui::TableHeadersRow();
        for (size_t neuron_index = 0; neuron_index != layer_biases.size(); ++neuron_index) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", neuron_index);
            ImGui::TableSetColumnIndex(1);
            ImGui::ColorConvertHSVtoRGB(0.17 * (std::tanh(layer_biases[neuron_index]) + 1), 1.0f, 1.0f, color.x,
                                        color.y, color.z);
            ImGui::TextColored(color, "%f", layer_biases[neuron_index]);
        }
        ImGui::EndTable();

        size_t start_index = 0;
        for (size_t remaining_columns = layer_weights.front().size(); remaining_columns != 0;) {
            const size_t columns_count =
                remaining_columns > IMGUI_TABLE_MAX_COLUMNS ? IMGUI_TABLE_MAX_COLUMNS : remaining_columns;
            const size_t next_start_index = start_index + columns_count;

            ImGui::SameLine(0, 0);
            snprintf(name_buffer, sizeof(name_buffer), "Weights from %zu", start_index);
            ImGui::BeginTable(name_buffer, columns_count,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoHostExtendX,
                              ImVec2(column_width * columns_count, 0));
            for (size_t weight_index = start_index; weight_index != next_start_index; ++weight_index) {
                snprintf(name_buffer, sizeof(name_buffer), "Weight %zu", weight_index);
                ImGui::TableSetupColumn(name_buffer);
            }
            ImGui::TableHeadersRow();

            for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
                const auto& neuron_weights = layer_weights[neuron_index];

                int column = 0;
                ImGui::TableNextRow();
                for (size_t input_index = start_index; input_index != next_start_index; ++input_index) {
                    ImGui::TableSetColumnIndex(column);
                    ImGui::ColorConvertHSVtoRGB(0.17 * (std::tanh(neuron_weights[input_index]) + 1), 1.0f, 1.0f,
                                                color.x, color.y, color.z);
                    ImGui::TextColored(color, "%f", neuron_weights[input_index]);
                    ++column;
                }
            }

            ImGui::EndTable();
            start_index = next_start_index;
            remaining_columns -= columns_count;
        }

        ImGui::EndChild();
        ImGui::TreePop();
    }
}

void ShowProperty(const Neural::Network& ann, std::vector<double>* inputs, std::vector<double>* outputs) {
    if (ImGui::TreeNode("Connections")) {
        DrawNetworkConnections(ann, inputs, outputs);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Layers")) {
        DrawNetworkLayers(ann);
        ImGui::TreePop();
    }
}

} // namespace Inspector
