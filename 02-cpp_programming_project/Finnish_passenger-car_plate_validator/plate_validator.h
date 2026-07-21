/*
=========================================================================================================================
 *** plate_validator.h ***
 Validator for the Finnish ABC-123 registration plate format, reporting the first rule that fails.
=========================================================================================================================

 Description:
     This header declares a small validation module for the standard Finnish passenger-car plate format: three
     uppercase letters, a dash, and three digits, seven characters in total. Unlike a boolean check, the validator
     reports a verdict enumerating the first rule that failed, which is what interactive and logging callers need:
     a user can be told that the length is wrong, that the letter field contains a lowercase or non-letter
     character, that the separator is missing, or that the digit field is not numeric, instead of a bare rejection.

     The rules are checked in a fixed order with length first, because the remaining checks index fixed positions
     and rely on the length holding. Character classification goes through the ctype predicates with the unsigned
     char cast, since passing a plain char with a negative value, which any non-ASCII input byte produces on
     platforms with signed char, is undefined behaviour.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/plate_validator.cpp
     tests       tests/test_plate.cpp (accept set, one test per failure reason, boundary cases)
=========================================================================================================================
*/
#ifndef PLATE_VALIDATOR_H
#define PLATE_VALIDATOR_H

#include <string_view>

namespace plate {

inline constexpr std::size_t kPlateLen    = 7;   // Format ABC-123.
inline constexpr std::size_t kLetterCount = 3;
inline constexpr std::size_t kDashPos     = 3;
inline constexpr std::size_t kDigitStart  = 4;
inline constexpr std::size_t kDigitCount  = 3;

enum class Verdict {
    Ok,
    BadLength,      // Not exactly seven characters.
    BadLetters,     // Positions 0-2 not all uppercase A-Z.
    BadSeparator,   // Position 3 is not a dash.
    BadDigits       // Positions 4-6 not all decimal digits.
};

// Validate and report the first failed rule (checked in the order above).
Verdict check(std::string_view s);

// Convenience boolean form.
bool is_valid(std::string_view s);

// Human-readable text for a verdict, for prompts and logs.
std::string_view describe(Verdict v);

}  // namespace plate

#endif  // PLATE_VALIDATOR_H
