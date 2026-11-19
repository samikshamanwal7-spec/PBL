#ifndef CURRENCY_H
#define CURRENCY_H

#include <string>

// ============================================================
// Currency.h -- single place that defines the currency symbol.
//
// The rupee sign (U+20B9) is written as its UTF-8 bytes so it
// compiles the same on every compiler. If your terminal font
// cannot display it, change SYMBOL to "Rs. " and rebuild.
// ============================================================
namespace currency {
    inline const std::string SYMBOL = "\xE2\x82\xB9"; // Indian rupee sign
}

#endif // CURRENCY_H
