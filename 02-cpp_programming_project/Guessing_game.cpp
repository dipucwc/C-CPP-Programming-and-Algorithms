/*
=========================================================================================================================
 *** guessing_game ***
 Interactive number-guessing game with a uniformly distributed secret and higher/lower feedback.
=========================================================================================================================

 Description:
     This program implements a guessing game in which the computer draws a secret integer between 0 and 100 and the
     player must find it. After every guess, the program reports whether the secret number is smaller or larger than
     the guess, so the player can close in on the answer by binary search. The game ends when the guess equals the
     secret, and the program reports the number of attempts used.

     The complete procedure runs in three parts. In the first part, the secret is drawn from a uniform integer
     distribution driven by a Mersenne Twister engine seeded from the hardware random device, so every run produces a
     different secret with equal probability across the range. In the second part, the input loop reads one guess per
     round, validates that the input is a number, and rejects non-numeric input without crashing or looping forever.
     In the third part, each valid guess is compared against the secret and the appropriate hint or the final success
     message is printed together with the attempt count.

 Input:
     stdin          One integer guess per round; non-numeric input is rejected and re-asked.

 Output:
     return value   Zero on normal completion.
     stdout         A prompt per round, a hint after each wrong guess, and a success line with the attempt count.

 Supporting files:
     header      <random> for std::random_device, std::mt19937, and std::uniform_int_distribution
=========================================================================================================================
*/
#include <iostream>
#include <limits>
#include <random>

namespace game {

constexpr int kMin = 0;
constexpr int kMax = 100;

int random_number(int low, int high) {
    // static: the engine is created and seeded once, then reused across calls.
    // Recreating it per call would reseed every time and weaken the randomness.
    static std::random_device dev;
    static std::mt19937 rng(dev());

    std::uniform_int_distribution<int> dist(low, high);
    return dist(rng);
}

int read_guess() {
    int guess = 0;
    while (!(std::cin >> guess)) {
        // Non-numeric input puts cin into a failed state and leaves the bad
        // characters in the buffer; both must be cleared or the loop spins forever.
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a whole number: ";
    }
    return guess;
}

}  // namespace game

int main() {
    const int secret = game::random_number(game::kMin, game::kMax);
    int attempts = 0;

    std::cout << "Guess the secret number between " << game::kMin
              << " and " << game::kMax << ".\n";

    while (true) {
        std::cout << "Guess a number: ";
        const int guess = game::read_guess();
        ++attempts;

        if (guess > secret) {
            std::cout << "The secret number is smaller!\n";
        } else if (guess < secret) {
            std::cout << "The secret number is larger!\n";
        } else {
            std::cout << "You found the secret number in "
                      << attempts << " attempts!\n";
            break;
        }
    }

    return 0;
}