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

constexpr std::size_t kPlateLen    = 7;   // Format ABC-123: 3 letters + dash + 3 digits.
constexpr std::size_t kLetterCount = 3;
constexpr std::size_t kDashPos     = 3;
constexpr std::size_t kDigitStart  = 4;
constexpr std::size_t kDigitCount  = 3;

// The cast is required: passing a plain char that is negative (non-ASCII input)
// to std::isupper/std::isdigit is undefined behaviour in C and C++.
bool is_upper(char c) {
    return std::isupper(static_cast<unsigned char>(c)) != 0;
}

bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool letters_ok(std::string_view s) {
    auto field = s.substr(0, kLetterCount);
    return std::all_of(field.begin(), field.end(), is_upper);
}

bool dash_ok(std::string_view s) {
    return s[kDashPos] == '-';
}

bool digits_ok(std::string_view s) {
    auto field = s.substr(kDigitStart, kDigitCount);
    return std::all_of(field.begin(), field.end(), is_digit);
}

bool is_valid(std::string_view s) {
    // Length must be checked first: the field checks below index fixed
    // positions and would read out of range on a shorter string.
    if (s.size() != kPlateLen) return false;
    return letters_ok(s) && dash_ok(s) && digits_ok(s);
}

}  // namespace plate

int main() {
    std::string line;

    std::cout << "Write a plate number: ";
    if (!std::getline(std::cin, line)) return 0;   // EOF: exit cleanly instead of using an empty string.

    std::cout << line << " is "
              << (plate::is_valid(line) ? "a valid" : "not a valid")
              << " plate number.\n";

    return 0;
}
