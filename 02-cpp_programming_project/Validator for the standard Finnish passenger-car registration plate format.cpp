/*
=========================================================================================================================
 *** check_finnish_plate ***
 Validator for the standard Finnish passenger-car registration plate format.
=========================================================================================================================

 Description:
     This program checks whether a string is a valid Finnish passenger-car registration plate. The standard format is
     three uppercase letters, a single dash, and three digits, for example ABC-123, giving a fixed length of seven
     characters. Four properties must all hold for the plate to be accepted: the length is exactly seven; the first
     three characters are uppercase letters A to Z; the fourth character is a dash; and the last three characters are
     digits 0 to 9. The purpose of the program is to turn a raw input line into a single clear verdict while keeping
     each rule in its own small, self-contained function.

     The complete procedure runs in four checks. In the first check, the length of the input is compared against the
     expected seven characters and any other length is rejected at once. In the second check, the letter field is
     scanned and every character is required to be an uppercase letter. In the third check, the separator position is
     required to hold a single dash. In the fourth check, the digit field is scanned and every character is required
     to be a decimal digit. The verdict is the logical AND of the four checks, so a failure in any one of them marks
     the plate as invalid. The program reads one line, applies the checks, and prints one verdict line.

 Input:
     stdin          One line of text read with std::getline, treated as the candidate plate.

 Output:
     return value   Zero on normal completion.
     stdout         One prompt line and one verdict line reporting VALID or INVALID.

 Supporting files:
     header      <cctype> for the character-class predicates, <algorithm> for std::all_of
     reference   Finnish Transport and Communications Agency plate format (three letters, dash, three digits)
=========================================================================================================================
*/
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>

namespace plate {

constexpr std::size_t kPlateLen    = 7;   // Total characters in a well-formed plate, e.g. ABC-123.
constexpr std::size_t kLetterCount = 3;   // Uppercase letters at the start of the plate.
constexpr std::size_t kDashPos     = 3;   // Zero-based index of the separating dash.
constexpr std::size_t kDigitStart  = 4;   // Zero-based index where the digit field begins.
constexpr std::size_t kDigitCount  = 3;   // Decimal digits at the end of the plate.

// is_upper: safe uppercase-letter test that avoids undefined behaviour on negative chars.
bool is_upper(char c) {
    return std::isupper(static_cast<unsigned char>(c)) != 0;   // Cast guards non-ASCII input.
}

// is_digit: safe decimal-digit test using the same defensive cast.
bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;   // Cast guards non-ASCII input.
}

// letters_ok: true when the first kLetterCount characters are all uppercase letters.
bool letters_ok(std::string_view s) {
    auto field = s.substr(0, kLetterCount);                    // The leading letter field.
    return std::all_of(field.begin(), field.end(), is_upper);  // Every character must be uppercase.
}

// dash_ok: true when the separator position holds a single dash.
bool dash_ok(std::string_view s) {
    return s[kDashPos] == '-';                                 // The fourth character must be a dash.
}

// digits_ok: true when the last kDigitCount characters are all decimal digits.
bool digits_ok(std::string_view s) {
    auto field = s.substr(kDigitStart, kDigitCount);           // The trailing digit field.
    return std::all_of(field.begin(), field.end(), is_digit);  // Every character must be a digit.
}

// is_valid: combine the length and field checks into one verdict.
bool is_valid(std::string_view s) {
    if (s.size() != kPlateLen) return false;                   // Wrong length is rejected first.
    return letters_ok(s) && dash_ok(s) && digits_ok(s);        // All three field checks must hold.
}

}  // namespace plate

int main() {
    std::string line;                                          // The input line.

    std::cout << "Write a plate number: ";                     // Prompt the user.
    if (!std::getline(std::cin, line)) return 0;               // Read one line; stop cleanly on EOF.

    std::cout << line << " is "                                // Report the single verdict.
              << (plate::is_valid(line) ? "a valid" : "not a valid")
              << " plate number.\n";

    return 0;                                                  // Normal completion.
}