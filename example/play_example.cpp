#include "replay.hpp"
#include <iostream>

int main() {
  try {
    // Example 1: Simple iteration
    std::cout << "Example 1: Simple data processing\n";
    std::cout << "=================================\n";

    Replay replay("example.csv");

    // Process all rows with a simple lambda
    replay.play([](const auto &json) {
      std::cout << "Timestamp: " << json["timestamp"]
                << ", Speed: " << json["speed"] << " km/h\n";
    });

    std::cout << "\nExample 2: Data aggregation\n";
    std::cout << "===========================\n";

    replay.reset(); // Start over

    // Calculate statistics
    double max_speed = 0;
    double total_distance = 0;
    int count = 0;

    replay.play([&](const auto &json) {
      double speed = json["speed"];
      max_speed = std::max(max_speed, speed);

      // Simulate distance calculation (speed * 1 second)
      total_distance += speed / 3600.0; // Convert to km
      count++;
    });

    std::cout << "Max speed: " << max_speed << " km/h\n";
    std::cout << "Total distance: " << total_distance << " km\n";
    std::cout << "Average speed: " << (total_distance * 3600.0 / count)
              << " km/h\n";

    std::cout << "\nExample 3: Filtering and processing\n";
    std::cout << "===================================\n";

    replay.reset();

    // Process only high-speed events
    replay.play([](const auto &json) {
      double speed = json["speed"];
      if (speed > 45.0) {
        std::cout << "High speed event: " << speed << " km/h at "
                  << "position (" << json["position"]["latitude"] << ", "
                  << json["position"]["longitude"] << ")\n";
      }
    });

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
