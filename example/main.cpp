#include "replay.hpp"
#include <iostream>

int main() {
  try {
    // Create a Replay instance with the example CSV file
    Replay replay("example.csv");

    std::cout << "Reading CSV file and converting to JSON:\n";
    std::cout << "========================================\n\n";

    int line_count = 1;

    // Process each line
    while (replay.has_next()) {
      auto json_obj = replay.advance();

      // Check if we got an empty object (end of file)
      if (json_obj.empty()) {
        break;
      }

      std::cout << "Line " << line_count << ":\n";
      std::cout << json_obj.dump(2)
                << "\n\n"; // Pretty print with 2-space indentation

      line_count++;
    }

    std::cout << "Finished processing " << (line_count - 1) << " lines.\n\n";

    // Demonstrate reset functionality
    std::cout << "Demonstrating reset functionality:\n";
    std::cout << "==================================\n";

    replay.reset();
    auto first_line_again = replay.advance();
    std::cout << "First line after reset:\n";
    std::cout << first_line_again.dump(2) << "\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
