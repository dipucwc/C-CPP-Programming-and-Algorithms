/*
=========================================================================================================================
 *** plate_validator.cpp ***
 Implementation of the ordered rule checks and the verdict descriptions.
=========================================================================================================================

 Description:
     This file implements the validator declared in plate_validator.h. Each rule is a small local predicate over the
     relevant field of the string view, evaluated in the documented order so the reported verdict is always the first
     failure a reader scanning left to right would meet. The character predicates wrap the ctype functions with the
     unsigned char cast that keeps non-ASCII input inside defined behaviour.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as values.

 Supporting files:
     header      include/plate_validator.h
=========================================================================================================================
*/
#include "plate_validator.h"

#include <algorithm>
#include <cctype>

namespace plate {

namespace {

// The cast is required: passing a plain char that is negative (any
// non-ASCII byte where char is signed) to std::isupper/std::isdigit
// is undefined behaviour in C and C++.
bool is_upper(char c) {
    return std::isupper(static_cast<unsigned char>(c)) != 0;
}

bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

}  // namespace

Verdict check(std::string_view s) {
    // Length first: the field checks below index fixed positions and
    // would read out of range on a shorter string.
    if (s.size() != kPlateLen) return Verdict::BadLength;

    const auto letters = s.substr(0, kLetterCount);
    if (!std::all_of(letters.begin(), letters.end(), is_upper))
        return Verdict::BadLetters;

    if (s[kDashPos] != '-') return Verdict::BadSeparator;

    const auto digits = s.substr(kDigitStart, kDigitCount);
    if (!std::all_of(digits.begin(), digits.end(), is_digit))
        return Verdict::BadDigits;

    return Verdict::Ok;
}

bool is_valid(std::string_view s) {
    return check(s) == Verdict::Ok;
}

std::string_view describe(Verdict v) {
    switch (v) {
        case Verdict::Ok:           return "valid";
        case Verdict::BadLength:    return "must be exactly 7 characters (ABC-123)";
        case Verdict::BadLetters:   return "first three characters must be uppercase letters";
        case Verdict::BadSeparator: return "fourth character must be a dash";
        case Verdict::BadDigits:    return "last three characters must be digits";
    }
    return "unknown";   // Unreachable with a valid enum value.
}

}  // namespace plate
