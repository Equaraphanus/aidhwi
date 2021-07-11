#pragma once

#include "neural/network.h"

#include <string>
#include <vector>

#include <imgui.h>

namespace Inspector {

bool ShowProperty(double&);
bool ShowProperty(int&);

void ShowProperty(const Neural::Network&, std::vector<double>* = nullptr, std::vector<double>* = nullptr);

} // namespace Inspector
