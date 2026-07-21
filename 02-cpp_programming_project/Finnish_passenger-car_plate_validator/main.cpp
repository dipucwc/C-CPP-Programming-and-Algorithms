/*
=========================================================================================================================
 *** main.cpp ***
 Interactive front end: read one candidate plate per line and report the verdict with its reason.
=========================================================================================================================

 Description:
     This program reads candidate plates line by line until end of input and prints, for each, either a confirmation
     or the first rule the candidate breaks, using the validator module's verdict descriptions. Reading until end of
     input makes the program usable both interactively and in a pipeline, where a file of candidates can be piped in
     and the verdicts collected.

 Input:
     stdin          One candidate plate per line.

 Output:
     return value   Zero on normal completion.
     stdout         One verdict line per candidate.

 Supporting files:
     header      include/plate_validator.h
=========================================================================================================================
*/
#include <iostream>
#include <string>

#include "plate_validator.h"

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto v = plate::check(line);
        if (v == plate::Verdict::Ok) {
            std::cout << line << ": valid\n";
        } else {
            std::cout << line << ": invalid - " << plate::describe(v) << "\n";
        }
    }
    return 0;
}
