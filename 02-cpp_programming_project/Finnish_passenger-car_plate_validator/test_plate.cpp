/*
=========================================================================================================================
 *** test_plate ***
 Unit tests: the accept set, one dedicated test per failure reason, rule ordering, and boundary characters.
=========================================================================================================================

 Description:
     This test verifies the validator through four properties. The first property is the accept set: a selection of
     well-formed plates spanning the letter and digit ranges must be accepted. The second property gives every
     failure reason its own dedicated case: wrong length in both directions, a lowercase letter, a digit in the
     letter field, a missing and a wrong separator, and a letter in the digit field, each required to return exactly
     its verdict. The third property pins the rule ordering: a candidate breaking several rules at once must report
     the first rule in the documented order, so length outranks letters, letters outrank the separator, and the
     separator outranks the digits. The fourth property checks boundary characters around the accepted ranges: the
     characters immediately before A, after Z, before 0, and after 9 in the character encoding must all be rejected,
     and non-ASCII bytes must be rejected without invoking undefined behaviour, which the unsigned char cast in the
     implementation guarantees.

 Input:
     (none)         All candidates are fixed in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed case and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/plate_validator.h (the functions under test)
=========================================================================================================================
*/
#include <cstdio>

#include "plate_validator.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("  FAILED: %s (line %d)\n", #cond, __LINE__);   \
            ++fails;                                                    \
        }                                                               \
    } while (0)

int main() {
    int fails = 0;
    using plate::Verdict;
    using plate::check;

    {   // Property 1: the accept set.
        CHECK(check("ABC-123") == Verdict::Ok);
        CHECK(check("AAA-000") == Verdict::Ok);
        CHECK(check("ZZZ-999") == Verdict::Ok);
        CHECK(plate::is_valid("XYZ-407"));
    }

    {   // Property 2: one case per failure reason.
        CHECK(check("")         == Verdict::BadLength);
        CHECK(check("ABC-12")   == Verdict::BadLength);
        CHECK(check("ABC-1234") == Verdict::BadLength);
        CHECK(check("aBC-123")  == Verdict::BadLetters);
        CHECK(check("1BC-123")  == Verdict::BadLetters);
        CHECK(check("ABC 123")  == Verdict::BadSeparator);
        CHECK(check("ABCX123")  == Verdict::BadSeparator);
        CHECK(check("ABC-12X")  == Verdict::BadDigits);
        CHECK(check("ABC--23")  == Verdict::BadDigits);
    }

    {   // Property 3: ordering when several rules break at once.
        CHECK(check("ab 12")    == Verdict::BadLength);     // Length wins.
        CHECK(check("abcx12x")  == Verdict::BadLetters);    // Letters beat separator.
        CHECK(check("ABCX12X")  == Verdict::BadSeparator);  // Separator beats digits.
    }

    {   // Property 4: boundary and non-ASCII characters.
        CHECK(check("@BC-123") == Verdict::BadLetters);     // '@' precedes 'A'.
        CHECK(check("[BC-123") == Verdict::BadLetters);     // '[' follows 'Z'.
        CHECK(check("ABC-/23") == Verdict::BadDigits);      // '/' precedes '0'.
        CHECK(check("ABC-:23") == Verdict::BadDigits);      // ':' follows '9'.
        // A non-ASCII byte in the letter field: must be rejected and,
        // via the unsigned char cast, without undefined behaviour.
        const char raw[] = {'\xC4', 'B', 'C', '-', '1', '2', '3', '\0'};
        CHECK(check(raw) == Verdict::BadLetters);
    }

    std::printf("[test_plate] accept + reasons + ordering + boundaries -> %s\n",
                fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
