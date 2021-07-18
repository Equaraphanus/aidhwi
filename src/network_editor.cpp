#include "network_editor.h"

#include "inspector.h"
#include "neural/network.h"

NetworkEditor::NetworkEditor(Neural::Network& ann, float learning_rate)
    : m_network(ann), m_learn_continuously(false), m_learning_rate(learning_rate),
      m_network_inputs(ann.GetWeights().front().front().size()), m_target_outputs(ann.GetWeights().back().size()) {}

NetworkEditor::~NetworkEditor() {}

void NetworkEditor::Show() {
    Inspector::ShowProperty(m_network, &m_network_inputs);
    if (ImGui::Button("Randomize")) {
        m_network.get().Randomize();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Learn", &m_learn_continuously);
    ImGui::SameLine();
    bool step_once = ImGui::Button("Step once");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 100);
    ImGui::SliderFloat("Learning rate", &m_learning_rate, 0.01f, 1.0f);
    if (ImGui::TreeNode("Target output")) {
        char name_buffer[32];
        for (size_t index = 0; index != m_target_outputs.size(); ++index) {
            sprintf_s(name_buffer, "Output %llu", index);
            ImGui::InputDouble(name_buffer, &m_target_outputs[index]);
        }
        ImGui::TreePop();
    }
    if (m_learn_continuously || step_once) {
        m_network.get().Learn(m_network_inputs, m_target_outputs, m_learning_rate);
    }
}

void NetworkEditor::Rebind(Neural::Network& new_network) {
    m_network = new_network;
    m_network_inputs.resize(new_network.GetWeights().front().front().size());
    m_target_outputs.resize(new_network.GetWeights().back().size());
}
