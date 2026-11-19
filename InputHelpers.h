#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <iostream>
#include <limits>
#include <string>
#include "common/Console.h"

// ============================================================
// InputHelpers.h -- shared console input used by every module's
// interface file (validated integer input and prompted text).
// ============================================================
namespace input {

inline int readInt() {
    int v;
    while (!(std::cin >> v)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        ui::error("Please enter a valid number.");
        std::cout << " > ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return v;
}

inline std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

} // namespace input

#endif // INPUT_HELPERS_H
