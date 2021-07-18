#pragma once

#include <functional>
#include <vector>

#include <neural/network.h>

class NetworkEditor {
public:
    NetworkEditor(Neural::Network&, float = 0.1f);
    ~NetworkEditor();

    void Show();

    void Rebind(Neural::Network&);

private:
    std::reference_wrapper<Neural::Network> m_network;
    bool m_learn_continuously;
    float m_learning_rate;
    std::vector<double> m_network_inputs;
    std::vector<double> m_target_outputs;
};
