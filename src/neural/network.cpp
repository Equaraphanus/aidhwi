#include "network.h"

#include <algorithm>
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

void Network::ComputeOutputForLayer(size_t layer_index, const std::vector<double>& inputs,
                                    std::vector<double>& outputs) const {
    assert(m_weights.size() == m_biases.size());
    assert(layer_index < m_weights.size());

    const auto& layer_weights = m_weights[layer_index];
    const auto& layer_biases = m_biases[layer_index];

    assert(inputs.size() >= layer_weights.front().size());
    assert(outputs.size() >= layer_weights.size());

    for (size_t neuron_index = 0; neuron_index != layer_weights.size(); ++neuron_index) {
        const auto& neuron_weights = layer_weights[neuron_index];
        auto& neuron_output = outputs[neuron_index];

        neuron_output = 0;
        for (size_t input_index = 0; input_index != neuron_weights.size(); ++input_index)
            neuron_output += inputs[input_index] * neuron_weights[input_index];
        neuron_output += layer_biases[neuron_index];
        neuron_output = ActivationFunction(neuron_output);
    }
}

std::vector<double> Network::ComputeOutput(const std::vector<double>& inputs) const {
    assert(m_weights.size() > 0);
    assert(m_weights.size() == m_biases.size());
    assert(inputs.size() == m_weights.front().front().size());

    std::vector<double> input_buffer(m_max_layer_size);
    input_buffer.assign(inputs.begin(), inputs.end());

    std::vector<double> output_buffer(m_max_layer_size);

    for (size_t layer_index = 0; layer_index != m_weights.size(); ++layer_index) {
        ComputeOutputForLayer(layer_index, input_buffer, output_buffer);
        // The outputs of this layer will become the inputs for the next one.
        output_buffer.swap(input_buffer);
    }

    // As the input and output buffers are swapped at the end of each iteration,
    // we have to return the input one as a result.
    input_buffer.resize(m_weights.back().size());
    return input_buffer;
}

void Network::Learn(const std::vector<double>& inputs, const std::vector<double>& target_outputs, double rate) {
    assert(rate > 0.0 && rate <= 1.0);
    assert(m_weights.size() > 0);
    assert(m_weights.size() == m_biases.size());
    assert(inputs.size() == m_weights.front().front().size());
    assert(target_outputs.size() == m_weights.back().size());

    // Perform the forward propagation pass and remember all the outputs.
    std::vector<std::vector<double>> outputs(m_weights.size());
    const auto* input_buffer = &inputs;
    for (size_t layer_index = 0; layer_index != m_weights.size(); ++layer_index) {
        auto& output_buffer = outputs[layer_index];
        output_buffer.resize(m_weights[layer_index].size());
        ComputeOutputForLayer(layer_index, *input_buffer, output_buffer);
        input_buffer = &output_buffer;
    }

    std::vector<double> error_buffer(m_max_layer_size);
    std::vector<double> next_error_buffer(m_max_layer_size);

    for (size_t output_index = 0; output_index != target_outputs.size(); ++output_index)
        error_buffer[output_index] = target_outputs[output_index] - outputs.back()[output_index];

    // Perform the backwards propagation pass and correct the weights according to the amount of error for each neuron.
    // The error values for the next layer are calculated on the fly.
    for (size_t layer_index = m_weights.size(); layer_index-- != 0;) {
        input_buffer = layer_index != 0 ? &outputs[layer_index - 1] : &inputs;
        auto& layer_weights = m_weights[layer_index];
        auto& layer_biases = m_biases[layer_index];
        const auto& layer_outputs = outputs[layer_index];
        std::fill_n(next_error_buffer.begin(), layer_weights.front().size(), 0);
        for (size_t output_index = 0; output_index != layer_weights.size(); ++output_index) {
            auto& output_weights = layer_weights[output_index];
            const auto& output_derivative = ActivationDerivativeFromValue(layer_outputs[output_index]);
            double error_value = error_buffer[output_index];
            for (size_t weight_index = 0; weight_index != output_weights.size(); ++weight_index) {
                auto& weight = output_weights[weight_index];
                // Contribute to the error value of each input before changing the weight.
                next_error_buffer[weight_index] += weight * error_value;
                // Correct the weight.
                weight += rate * error_value * output_derivative * input_buffer->at(weight_index);
            }
            // Bias is a special case as it does not contribute to any error value.
            layer_biases[output_index] += rate * error_value * output_derivative;
        }
        error_buffer.swap(next_error_buffer);
    }
}

double Network::ActivationFunction(double x) {
    // Displaced tanh seems to be pretty fast and relatively easy to differentiate.
    return 0.5 * (std::tanh(x) + 1);
}

double Network::ActivationDerivativeFromValue(double y) {
    return 2.0 * y * (1.0 - y);
}

} // namespace Neural
