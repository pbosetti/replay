/*
 * Simple example showing how to use the header-only Replay class.
 *
 * To compile and run:
 *   g++ -std=c++17 simple_example.cpp -o simple_example && ./simple_example
 *
 * Note: Make sure nlohmann/json is available. On macOS with Homebrew:
 *   brew install nlohmann-json
 *   g++ -std=c++17 -I$(brew --prefix nlohmann-json)/include simple_example.cpp
 * -o simple_example
 */

#include "replay.hpp"
#include <iostream>

int main() {
  try {
    // Method 1: Using the play() method (recommended)
    std::cout << "Method 1: Using play() method\n";
    std::cout << "=============================\n";

    Replay replay("example.csv");

    int count = 0;
    replay.play([&count](const auto &json) {
      count++;
      std::cout << "Row " << count << ": "
                << "Speed=" << json["speed"]
                << ", X-acceleration=" << json["acceleration"]["x"]
                << ", Signals=" << json["signal"].dump() << "\n";
    });

    std::cout << "Processed " << count << " rows with play() method.\n\n";

    // Method 2: Manual iteration (traditional approach)
    std::cout << "Method 2: Manual iteration\n";
    std::cout << "=========================\n";

    replay.reset();
    count = 0;

    while (replay.has_next()) {
      auto json = replay.advance();
      if (json.empty())
        break;

      count++;
      std::cout << "Row " << count << ": Speed=" << json["speed"] << "\n";
    }

    std::cout << "Processed " << count << " rows with manual iteration.\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
