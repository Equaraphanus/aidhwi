#include "inspector.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "imgui.h"
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
        ImGui::Text("Weight %llu", input_index);
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
        input_buffer->assign(inputs->begin(), inputs->end());
        output_buffer.emplace(max_layer_size);
    }

    ImGui::BeginGroup();
    const size_t inputs_count = weights.front().front().size();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset * (max_layer_size - inputs_count) * 0.5f);
    for (size_t input_index = 0; input_index != inputs_count; ++input_index) {
        if (inputs) {
            sprintf_s(name_buffer, "Input %llu", input_index);
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
            sprintf_s(name_buffer, "%llu", input_index);
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
            sprintf_s(name_buffer, "%llu", neuron_index);
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
        if (!ImGui::TreeNode((void*)(intptr_t)layer_index, "Layer %llu", layer_index))
            continue;

        const auto& layer_weights = weights[layer_index];
        const auto& layer_biases = biases[layer_index];

        for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
            if (!ImGui::TreeNode((void*)(intptr_t)neuron_index, "Neuron %llu", neuron_index))
                continue;

            ImGui::BeginTable("Values", 2, ImGuiTableFlags_RowBg);

            const auto& neuron_weights = layer_weights[neuron_index];
            for (size_t input_index = 0; input_index != neuron_weights.size(); ++input_index) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Weight %llu", input_index);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", neuron_weights[input_index]);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Bias");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%f", layer_biases[neuron_index]);

            ImGui::EndTable();

            ImGui::TreePop();
        }

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
