#include "../src/replay.hpp"
#include <iostream>

int main() {
  try {
    std::cout << "Testing the new play() method with lambdas:\n";
    std::cout << "===========================================\n\n";

    // Test 1: Simple lambda that prints each JSON object
    std::cout << "Test 1: Simple JSON dumping\n";
    std::cout << "----------------------------\n";
    Replay replay1("example.csv");

    int count = 0;
    replay1.play([&count](const auto &json) {
      count++;
      std::cout << "Row " << count << ": " << json.dump() << "\n\n";
    });

    std::cout << "Processed " << count << " rows with simple lambda.\n\n";

    // Test 2: Lambda that processes specific fields
    std::cout << "Test 2: Field-specific processing\n";
    std::cout << "----------------------------------\n";
    Replay replay2("example.csv");

    double total_speed = 0;
    int speed_count = 0;

    replay2.play([&total_speed, &speed_count](const auto &json) {
      if (json.contains("speed")) {
        total_speed += static_cast<double>(json["speed"]);
        speed_count++;

        std::cout << "Speed: " << json["speed"]
                  << ", Driver: " << json["driver"]["name"]
                  << ", X-accel: " << json["acceleration"]["x"] << "\n";
      }
    });

    std::cout << "Average speed: " << (total_speed / speed_count)
              << " km/h\n\n";

    // Test 3: Lambda with CSV file containing comments
    std::cout << "Test 3: Processing file with comments\n";
    std::cout << "-------------------------------------\n";
    Replay replay3("example_with_comments.csv");

    std::vector<double> timestamps;

    replay3.play([&timestamps](const auto &json) {
      if (json.contains("timestamp")) {
        timestamps.push_back(static_cast<double>(json["timestamp"]));
      }
    });

    std::cout << "Collected " << timestamps.size() << " timestamps: [";
    for (size_t i = 0; i < timestamps.size(); ++i) {
      std::cout << timestamps[i];
      if (i < timestamps.size() - 1)
        std::cout << ", ";
    }
    std::cout << "]\n\n";

    // Test 4: Chaining - reset and play again
    std::cout << "Test 4: Reset and play again\n";
    std::cout << "-----------------------------\n";

    replay3.reset();
    int second_count = 0;
    replay3.play([&second_count](const auto &) { second_count++; });

    std::cout << "Second pass processed " << second_count
              << " rows (should be same as first pass)\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
