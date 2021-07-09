#pragma once

#include <cstdint>
#include <vector>
#include <initializer_list>

namespace Neural {

class Network {
public:
    Network(size_t, const std::vector<size_t>&);
    ~Network();

    void Randomize(std::uint64_t);
    // Randomize using current time as a seed.
    void Randomize();

    std::vector<double> ComputeOutput(const std::vector<double>&) const;

private:
    std::vector<std::vector<std::vector<double>>> m_weights;
    std::vector<std::vector<double>> m_biases;
    size_t m_max_layer_size;

    static double ActivationFunction(double);
    static double ActivationDerivative(double);
};

} // namespace Neural
