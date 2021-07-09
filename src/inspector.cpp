#include "inspector.h"

#include "imgui.h"
#include "neural/network.h"
#include <cmath>
#include <string>
#include <vector>

namespace Inspector {

static bool RoundButton(const std::string& id, float size = 32) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, size / 2);
    bool result = ImGui::Button(id.c_str(), ImVec2(size, size));
    ImGui::PopStyleVar();
    return result;
}

static bool NeuronWidget(const std::string& id, double bias, std::vector<double> weights, float size = 32) {
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

    if (ImGui::IsItemHovered()) {
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
        ImGui::EndTable();
        ImGui::EndTooltip();
    }

    return result;
}

static void DrawNetworkConnections(const Neural::Network& ann) {
    const auto& weights = ann.GetWeights();
    const auto& biases = ann.GetBiases();

    char name_buffer[32];

    ImGui::BeginGroup();
    for (size_t input_index = 0; input_index != weights.front().front().size(); ++input_index) {
        sprintf_s(name_buffer, "%llu", input_index);
        RoundButton(name_buffer);
    }
    ImGui::EndGroup();

    for (size_t layer_index = 0; layer_index != weights.size(); ++layer_index) {

        const auto& layer_weights = weights[layer_index];
        const auto& layer_biases = biases[layer_index];

        ImGui::SameLine();
        ImGui::BeginGroup();
        for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
            sprintf_s(name_buffer, "%llu", neuron_index);
            NeuronWidget(name_buffer, layer_biases[neuron_index], layer_weights[neuron_index]);
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

void ShowProperty(const Neural::Network& ann) {
    if (ImGui::TreeNode("Connections")) {
        DrawNetworkConnections(ann);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Layers")) {
        DrawNetworkLayers(ann);
        ImGui::TreePop();
    }
}

} // namespace Inspector
