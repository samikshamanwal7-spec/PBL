#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// ============================================================
// Console.h
// Lightweight helpers to make a plain terminal feel like a
// small "desktop" application window: colored text, boxed
// panels, and aligned tables. Pure standard library + ANSI
// escape codes, so it compiles and runs anywhere (no external
// GUI toolkit required) -> keeps the project truly offline.
// ============================================================


namespace ui {

// ---- ANSI colors ----
inline const std::string RESET   = "\033[0m";
inline const std::string BOLD    = "\033[1m";
inline const std::string RED     = "\033[31m";
inline const std::string GREEN   = "\033[32m";
inline const std::string YELLOW  = "\033[33m";
inline const std::string BLUE    = "\033[34m";
inline const std::string MAGENTA = "\033[35m";
inline const std::string CYAN    = "\033[36m";
inline const std::string WHITE   = "\033[37m";

inline void clearScreen() {
    // Works on Windows terminals (VT100 enabled by default on Win10+),
    // Linux and macOS terminals.
    std::cout << "\033[2J\033[1;1H";
}

inline void pause() {
    std::cout << YELLOW << "\nPress ENTER to continue..." << RESET;
    std::cin.clear();
    std::string dummy;
    std::getline(std::cin, dummy);
}

inline void drawLine(int width, char c = '-') {
    std::cout << std::string(width, c) << "\n";
}

inline void title(const std::string& text) {
    int width = 60;
    std::cout << CYAN << BOLD;
    drawLine(width, '=');
    int pad = (width - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    std::cout << std::string(pad, ' ') << text << "\n";
    drawLine(width, '=');
    std::cout << RESET;
}

inline void banner(const std::string& appName) {
    clearScreen();
    title(appName);
    std::cout << "\033[2m" << "  Learn trading with virtual money..." << RESET << "\n\n";
}

inline void success(const std::string& msg) {
    std::cout << GREEN << BOLD << "[OK] " << RESET << GREEN << msg << RESET << "\n";
}

inline void error(const std::string& msg) {
    std::cout << RED << BOLD << "[ERROR] " << RESET << RED << msg << RESET << "\n";
}

inline void info(const std::string& msg) {
    std::cout << CYAN << "[INFO] " << RESET << msg << "\n";
}

// A simple boxed menu. Returns nothing; just prints.
inline void menuBox(const std::string& heading, const std::list<std::string>& options) {
    size_t width = heading.size();
    for (auto& o : options) width = std::max(width, o.size());
    width += 8;

    std::cout << BLUE << "+" << std::string(width, '-') << "+\n";
    std::cout << "| " << BOLD << heading << RESET << BLUE
              << std::string(width - heading.size() - 1, ' ') << "|\n";
    std::cout << "+" << std::string(width, '-') << "+" << RESET << "\n";
    size_t i = 0;
    for (const auto& opt : options) {
        std::cout << WHITE << " [" << (++i) << "] " << opt << RESET << "\n";
    }
    std::cout << "\n" << WHITE << " > " << RESET;
}

// Prints a value in green if >= 0, red if negative, with a sign.
inline std::string colorSigned(double value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision);
    if (value >= 0) oss << GREEN << "+" << value << RESET;
    else            oss << RED << value << RESET;
    return oss.str();
}

} // namespace ui

#endif // CONSOLE_H
