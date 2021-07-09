#pragma once

#include "neural/network.h"
#include <string>

#include <imgui.h>

namespace Inspector {

bool ShowProperty(double&);
bool ShowProperty(int&);

void ShowProperty(const Neural::Network&);

} // namespace Inspector
