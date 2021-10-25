#pragma once

#include <functional>
#include <string>
#include <vector>

#include <neural/network.h>

class NetworkEditor {
public:
    NetworkEditor(Neural::Network&, float = 0.1f);
    ~NetworkEditor();

    void Show();

    void Rebind(Neural::Network&);

    template <typename T>
    inline void SetInputs(const std::vector<T>& new_inputs) {
        std::copy(new_inputs.begin(), new_inputs.begin() + std::min(m_network_inputs.size(), new_inputs.size()),
                  m_network_inputs.begin());
    }

    inline const auto& GetInputs() const { return m_network_inputs; }

    template <typename IT, typename OT>
    inline void AddLearningExampleRecord(const std::vector<IT>& inputs, const std::vector<OT>& outputs) {
        m_dataset_records.emplace_back(inputs, outputs);
    }

    bool LoadLearningExamples(const std::string&);
    bool SaveLearningExamples(const std::string&) const;

private:
    struct Record {
        std::vector<double> inputs;
        std::vector<double> outputs;

        template <typename IT, typename OT>
        Record(const std::vector<IT>& input_values, const std::vector<OT>& output_values)
            : inputs(input_values.begin(), input_values.end()), outputs(output_values.begin(), output_values.end()) {}
    };

    std::reference_wrapper<Neural::Network> m_network;
    bool m_learn_continuously;
    float m_learning_rate;
    std::vector<double> m_network_inputs;
    std::vector<Record> m_dataset_records;
    std::string m_model_save_path;
    std::string m_dataset_save_path;
    bool m_wants_write;
    bool m_wants_model;
};
