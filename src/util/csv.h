#pragma once

#include <istream>

namespace Csv {

template <char C>
inline void ConsumeSeparator(std::istream& in) {
    if (in.peek() == C)
        in.ignore();
}

} // namespace Csv
