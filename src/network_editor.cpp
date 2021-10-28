#include "network_editor.h"

#include <cstring>
#include <filesystem>
#include <fstream>

#include "imgui.h"
#include "inspector.h"
#include <util/csv.h>

NetworkEditor::NetworkEditor(Neural::Network& ann, float learning_rate)
    : m_network(ann), m_learn_continuously(false), m_learning_rate(learning_rate),
      m_network_inputs(ann.GetWeights().front().front().size()), m_model_save_path(256, '\0'),
      m_dataset_save_path(256, '\0') {}

NetworkEditor::~NetworkEditor() {}

void NetworkEditor::Show() {
    Inspector::ShowProperty(m_network, &m_network_inputs);

    bool wants_action = false;

    if (ImGui::TreeNode("Learning dataset")) {
        if (ImGui::Button("Load")) {
            wants_action = true;
            m_wants_model = false;
            m_wants_write = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            wants_action = true;
            m_wants_model = false;
            m_wants_write = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 100);
        ImGui::InputText("Path", m_dataset_save_path.data(), m_dataset_save_path.capacity());

        char name_buffer[32];
        for (size_t record_index = 0; record_index != m_dataset_records.size(); ++record_index) {
            auto& record = m_dataset_records[record_index];
            snprintf(name_buffer, sizeof(name_buffer), "Record %zu", record_index);
            if (ImGui::TreeNode(name_buffer)) {
                if (ImGui::TreeNode("Inputs")) {
                    for (size_t input_index = 0; input_index != record.inputs.size(); ++input_index) {
                        snprintf(name_buffer, sizeof(name_buffer), "Input %zu", input_index);
                        ImGui::InputDouble(name_buffer, &record.inputs[input_index]);
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Outputs")) {
                    for (size_t output_index = 0; output_index != record.outputs.size(); ++output_index) {
                        snprintf(name_buffer, sizeof(name_buffer), "Output %zu", output_index);
                        ImGui::InputDouble(name_buffer, &record.outputs[output_index]);
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    if (ImGui::Button("Randomize")) {
        m_network.get().Randomize();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        // TODO: m_network.get() = LoadFromFile(m_model_save_path);
        wants_action = true;
        m_wants_model = true;
        m_wants_write = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        // TODO: m_network.get().DumpToFile(m_model_save_path);
        wants_action = true;
        m_wants_model = true;
        m_wants_write = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 100);
    ImGui::InputText("Path", m_model_save_path.data(), m_model_save_path.capacity());

    ImGui::Checkbox("Learn", &m_learn_continuously);
    ImGui::SameLine();
    bool step_once = ImGui::Button("Step once");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 100);
    ImGui::SliderFloat("Learning rate", &m_learning_rate, 0.01f, 1.0f);
    if (m_learn_continuously || step_once) {
        for (const auto& record : m_dataset_records) {
            m_network.get().Learn(record.inputs, record.outputs, m_learning_rate);
        }
    }

    if (wants_action && m_wants_write) {
        if (std::filesystem::exists(m_wants_model ? m_model_save_path : m_dataset_save_path))
            ImGui::OpenPopup("Warning");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Warning")) {
        wants_action = false;

        ImGui::Text("File \"%s\" already exists. Overwrite?",
                    m_wants_model ? m_model_save_path.c_str() : m_dataset_save_path.c_str());
        ImGui::Separator();

        if (ImGui::Button("Yes")) {
            wants_action = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (wants_action) {
        if (!(m_wants_write ? (m_wants_model ? false : SaveLearningExamples(m_dataset_save_path))
                             : (m_wants_model ? false : LoadLearningExamples(m_dataset_save_path))))
            ImGui::OpenPopup("Error");
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Error")) {
        ImGui::Text("Failed to open \"%s\" for %s.",
                    m_wants_model ? m_model_save_path.c_str() : m_dataset_save_path.c_str(),
                    m_wants_write ? "writing" : "reading");
        ImGui::Separator();

        ImGui::SetItemDefaultFocus();
        if (ImGui::Button(":^("))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void NetworkEditor::Rebind(Neural::Network& new_network) {
    if ((m_network.get().GetInputsCount() != new_network.GetInputsCount()) ||
        (m_network.get().GetOutputsCount() != new_network.GetOutputsCount()))
        m_dataset_records.clear();

    m_network = new_network;
    m_network_inputs.resize(new_network.GetWeights().front().front().size());
}

bool NetworkEditor::LoadLearningExamples(const std::string& path) {
    std::ifstream input_stream(path);
    if (!input_stream.is_open())
        return false;

    std::vector<double> inputs(m_network.get().GetInputsCount());
    std::vector<double> outputs(m_network.get().GetOutputsCount());

    for (;;) {
        for (auto& input : inputs) {
            input_stream >> input;
            Csv::ConsumeSeparator<','>(input_stream);
        }
        for (auto& output : outputs) {
            input_stream >> output;
            Csv::ConsumeSeparator<','>(input_stream);
        }
        if (!input_stream.good())
            break;

        m_dataset_records.emplace_back(inputs, outputs);
    }

    input_stream.close();
    return true;
}

bool NetworkEditor::SaveLearningExamples(const std::string& path) const {
    std::ofstream output_stream(path);
    if (!output_stream.is_open())
        return false;

    output_stream.precision(20);

    for (const auto& record : m_dataset_records) {
        for (const auto& input_value : record.inputs)
            output_stream << input_value << ", ";

        auto outputs_begin = record.outputs.cbegin();
        auto outputs_end = record.outputs.cend();
        for (auto output_iterator = outputs_begin; output_iterator != outputs_end; ++output_iterator) {
            if (output_iterator != outputs_begin)
                output_stream << ", ";
            output_stream << *output_iterator;
        }

        output_stream << std::endl;
    }

    output_stream.close();
    return true;
}
