#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace Neural {

class Network {
public:
    Network(size_t, const std::vector<size_t>&);
    ~Network();

    void Randomize(std::uint64_t);
    // Randomize using current time as a seed.
    void Randomize();

    void ComputeOutputForLayer(size_t, const std::vector<double>&, std::vector<double>&) const;

    std::vector<double> ComputeOutput(const std::vector<double>&) const;

    void Learn(const std::vector<double>&, const std::vector<double>&, double);

    inline const auto& GetWeights() const { return m_weights; }
    inline const auto& GetBiases() const { return m_biases; }

    inline size_t GetMaxLayerSize() const { return m_max_layer_size; }

    inline size_t GetInputsCount() const { return m_weights.front().front().size(); }
    inline size_t GetOutputsCount() const { return m_weights.back().size(); }

private:
    std::vector<std::vector<std::vector<double>>> m_weights;
    std::vector<std::vector<double>> m_biases;
    size_t m_max_layer_size;

    static double ActivationFunction(double);
    static double ActivationDerivativeFromValue(double);
};

} // namespace Neural
