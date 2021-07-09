#include "network.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <vector>

#include <util/random.h>

namespace Neural {

Network::Network(size_t inputs_count, const std::vector<size_t>& layer_sizes)
    : m_weights(), m_biases(), m_max_layer_size(inputs_count) {
    assert(layer_sizes.size() > 0);

    m_weights.reserve(layer_sizes.size());
    m_biases.reserve(m_weights.size());
    for (auto layer_size : layer_sizes) {
        m_weights.emplace_back(layer_size, std::vector<double>(inputs_count));
        m_biases.emplace_back(layer_size);
        inputs_count = layer_size;

        if (m_max_layer_size < layer_size)
            m_max_layer_size = layer_size;
    }
}

Network::~Network() {}

void Network::Randomize(std::uint64_t seed) {
    Random::Prng rng(seed);
    // Randomize weights
    for (auto& layer : m_weights)
        for (auto& neuron : layer)
            for (auto& weight : neuron)
                weight = rng.NextFloat<double>(-1, 1);
    // Randomize biases
    for (auto& layer : m_biases)
        for (auto& bias : layer)
            bias = rng.NextFloat<double>(-1, 1);
}

void Network::Randomize() {
    Randomize(std::time(nullptr));
}

std::vector<double> Network::ComputeOutput(const std::vector<double>& inputs) const {
    assert(m_weights.size() > 0);
    assert(m_weights.size() == m_biases.size());
    assert(inputs.size() == m_weights.front().front().size());

    std::vector<double> input_buffer(m_max_layer_size);
    input_buffer.assign(inputs.begin(), inputs.end());

    std::vector<double> output_buffer(m_max_layer_size);

    for (size_t layer_index = 0; layer_index != m_weights.size(); ++layer_index) {
        const auto& layer_weights = m_weights[layer_index];
        const auto& layer_biases = m_biases[layer_index];

        for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
            const auto& neuron_weights = layer_weights[neuron_index];
            auto& neuron_output = output_buffer[neuron_index];

            neuron_output = 0;
            for (size_t input_index = 0; input_index != neuron_weights.size(); ++input_index)
                neuron_output += input_buffer[input_index] * neuron_weights[input_index];
            neuron_output += layer_biases[neuron_index];
            neuron_output = ActivationFunction(neuron_output);
        }

        // The outputs of this layer will become the inputs for the next one.
        output_buffer.swap(input_buffer);
    }

    // As the input and output buffers are swapped at the end of each iteration,
    // we have to return the input one as a result.
    input_buffer.resize(m_weights.back().size());
    return input_buffer;
}

double Network::ActivationFunction(double x) {
    // Displaced tanh seems to be pretty fast and relatively easy to differentiate.
    return 0.5 * (std::tanh(x) + 1);
}

double Network::ActivationDerivative(double x) {
    return 1.0 / (std::cosh(2 * x) - 1);
}

} // namespace Neural
