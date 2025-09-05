#include "../src/replay.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

// Simple test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name)                                                         \
  do {                                                                         \
    std::cout << "Running " #name "... ";                                      \
    try {                                                                      \
      test_##name();                                                           \
      std::cout << "PASSED\n";                                                 \
      tests_passed++;                                                          \
    } catch (const std::exception &e) {                                        \
      std::cout << "FAILED: " << e.what() << "\n";                             \
      tests_failed++;                                                          \
    } catch (...) {                                                            \
      std::cout << "FAILED: Unknown exception\n";                              \
      tests_failed++;                                                          \
    }                                                                          \
    tests_total++;                                                             \
  } while (0)

#define ASSERT_EQ(expected, actual)                                            \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      std::stringstream ss;                                                    \
      ss << "Assertion failed: expected " << (expected) << ", got "            \
         << (actual) << " at line " << __LINE__;                               \
      throw std::runtime_error(ss.str());                                      \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::stringstream ss;                                                    \
      ss << "Assertion failed: " #condition " is false at line " << __LINE__;  \
      throw std::runtime_error(ss.str());                                      \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition)                                                \
  do {                                                                         \
    if (condition) {                                                           \
      std::stringstream ss;                                                    \
      ss << "Assertion failed: " #condition " is true at line " << __LINE__;   \
      throw std::runtime_error(ss.str());                                      \
    }                                                                          \
  } while (0)

#define ASSERT_THROWS(statement, exception_type)                               \
  do {                                                                         \
    bool threw = false;                                                        \
    try {                                                                      \
      statement;                                                               \
    } catch (const exception_type &) {                                         \
      threw = true;                                                            \
    }                                                                          \
    if (!threw) {                                                              \
      std::stringstream ss;                                                    \
      ss << "Assertion failed: expected " #statement                           \
            " to throw " #exception_type                                       \
         << " at line " << __LINE__;                                           \
      throw std::runtime_error(ss.str());                                      \
    }                                                                          \
  } while (0)

int tests_total = 0;
int tests_passed = 0;
int tests_failed = 0;

// Test basic functionality
TEST(basic_csv_parsing) {
  Replay replay("example.csv");

  // Test has_next
  ASSERT_TRUE(replay.has_next());

  // Test advance
  auto json = replay.advance();
  ASSERT_FALSE(json.empty());
  ASSERT_TRUE(json.contains("timestamp"));
  ASSERT_TRUE(json.contains("speed"));
  ASSERT_TRUE(json.contains("driver"));

  // Test nested objects
  ASSERT_TRUE(json["driver"].contains("name"));
  ASSERT_TRUE(json["driver"].contains("age"));
  ASSERT_TRUE(json["acceleration"].contains("x"));
  ASSERT_TRUE(json["acceleration"].contains("y"));
  ASSERT_TRUE(json["acceleration"].contains("z"));

  // Test arrays
  ASSERT_TRUE(json["signal"].is_array());
  ASSERT_EQ(3, json["signal"].size());
}

TEST(nested_objects) {
  Replay replay("example.csv");
  auto json = replay.advance();

  // Test nested object structure
  ASSERT_EQ(2.5, json["acceleration"]["x"]);
  ASSERT_EQ(1.3, json["acceleration"]["y"]);
  ASSERT_EQ(-0.8, json["acceleration"]["z"]);

  ASSERT_EQ("John Doe", json["driver"]["name"]);
  ASSERT_EQ(35.0, json["driver"]["age"]);

  ASSERT_EQ(37.7749, json["position"]["latitude"]);
  ASSERT_EQ(-122.4194, json["position"]["longitude"]);
}

TEST(array_parsing) {
  Replay replay("example.csv");
  auto json = replay.advance();

  // Test array values
  ASSERT_TRUE(json["signal"].is_array());
  ASSERT_EQ(3, json["signal"].size());
  ASSERT_EQ(101.0, json["signal"][0]);
  ASSERT_EQ(102.0, json["signal"][1]);
  ASSERT_EQ(103.0, json["signal"][2]);
}

TEST(multiple_rows) {
  Replay replay("example.csv");

  std::vector<double> timestamps;
  std::vector<double> speeds;

  while (replay.has_next()) {
    auto json = replay.advance();
    if (json.empty())
      break;

    timestamps.push_back(json["timestamp"]);
    speeds.push_back(json["speed"]);
  }

  ASSERT_EQ(4, timestamps.size());
  ASSERT_EQ(4, speeds.size());

  // Check specific values
  ASSERT_EQ(1609459200.0, timestamps[0]);
  ASSERT_EQ(1609459201.0, timestamps[1]);
  ASSERT_EQ(45.2, speeds[0]);
  ASSERT_EQ(47.8, speeds[1]);
}

TEST(reset_functionality) {
  Replay replay("example.csv");

  // Read first row
  auto first_json = replay.advance();
  ASSERT_EQ(1609459200.0, first_json["timestamp"]);

  // Read second row
  auto second_json = replay.advance();
  ASSERT_EQ(1609459201.0, second_json["timestamp"]);

  // Reset and read first row again
  replay.reset();
  auto first_again = replay.advance();
  ASSERT_EQ(1609459200.0, first_again["timestamp"]);
}

TEST(comment_line_skipping) {
  Replay replay("example_with_comments.csv");

  std::vector<double> timestamps;

  while (replay.has_next()) {
    auto json = replay.advance();
    if (json.empty())
      break;
    timestamps.push_back(json["timestamp"]);
  }

  // Should still get 4 data rows despite comments
  ASSERT_EQ(4, timestamps.size());
  ASSERT_EQ(1609459200.0, timestamps[0]);
  ASSERT_EQ(1609459203.0, timestamps[3]);
}

TEST(edge_case_comments) {
  Replay replay("edge_case_comments.csv");

  std::vector<std::string> names;
  std::vector<double> values;

  while (replay.has_next()) {
    auto json = replay.advance();
    if (json.empty())
      break;
    names.push_back(json["name"]);
    values.push_back(json["timestamp"]);
  }

  ASSERT_EQ(3, names.size());
  ASSERT_EQ("Alice", names[0]);
  ASSERT_EQ("Bob", names[1]);
  ASSERT_EQ("Charlie", names[2]);

  ASSERT_EQ(100.0, values[0]);
  ASSERT_EQ(200.0, values[1]);
  ASSERT_EQ(300.0, values[2]);
}

TEST(play_method_basic) {
  Replay replay("example.csv");

  int count = 0;
  std::vector<double> speeds;

  replay.play([&count, &speeds](const auto &json) {
    count++;
    speeds.push_back(json["speed"]);
  });

  ASSERT_EQ(4, count);
  ASSERT_EQ(4, speeds.size());
  ASSERT_EQ(45.2, speeds[0]);
  ASSERT_EQ(49.6, speeds[3]);
}

TEST(play_method_with_filtering) {
  Replay replay("example.csv");

  std::vector<double> high_speeds;

  replay.play([&high_speeds](const auto &json) {
    double speed = json["speed"];
    if (speed > 45.0) {
      high_speeds.push_back(speed);
    }
  });

  ASSERT_EQ(3, high_speeds.size()); // Should be 45.2, 47.8, 49.6
  ASSERT_EQ(45.2, high_speeds[0]);
  ASSERT_EQ(47.8, high_speeds[1]);
  ASSERT_EQ(49.6, high_speeds[2]);
}

TEST(play_method_with_reset) {
  Replay replay("example.csv");

  int first_count = 0;
  replay.play([&first_count](const auto &) { first_count++; });

  replay.reset();
  int second_count = 0;
  replay.play([&second_count](const auto &) { second_count++; });

  ASSERT_EQ(first_count, second_count);
  ASSERT_EQ(4, first_count);
}

TEST(file_not_found) {
  ASSERT_THROWS(Replay("nonexistent.csv"), std::runtime_error);
}

TEST(empty_json_at_end) {
  Replay replay("example.csv");

  // Read all rows
  while (replay.has_next()) {
    auto json = replay.advance();
    if (json.empty())
      break;
  }

  // Should return empty JSON when no more data
  auto empty_json = replay.advance();
  ASSERT_TRUE(empty_json.empty());
}

TEST(type_conversion) {
    Replay replay("example.csv");
    auto json = replay.advance();
    
    // Numbers should be parsed as numbers
    ASSERT_TRUE(json["timestamp"].is_number());
    ASSERT_TRUE(json["speed"].is_number());
    ASSERT_TRUE(json["acceleration"]["x"].is_number());
    ASSERT_TRUE(json["driver"]["age"].is_number());
    
    // Strings should remain strings
    ASSERT_TRUE(json["driver"]["name"].is_string());
    
    // Arrays should be arrays
    ASSERT_TRUE(json["signal"].is_array());
}

TEST(loop_functionality_disabled) {
    Replay replay("example.csv");
    
    // Initially loop should be disabled
    ASSERT_FALSE(replay.is_loop_enabled());
    
    // Read all 4 rows
    std::vector<double> timestamps;
    while (replay.has_next()) {
        auto json = replay.advance();
        if (json.empty()) break;
        timestamps.push_back(json["timestamp"]);
    }
    
    ASSERT_EQ(4, timestamps.size());
    
    // Next advance should return empty JSON
    auto empty_json = replay.advance();
    ASSERT_TRUE(empty_json.empty());
}

TEST(loop_functionality_enabled) {
    Replay replay("example.csv");
    replay.set_loop(true);
    
    ASSERT_TRUE(replay.is_loop_enabled());
    ASSERT_TRUE(replay.has_next());  // Should always return true in loop mode
    
    // Read more than 4 rows to verify looping
    std::vector<double> timestamps;
    for (int i = 0; i < 8; i++) {
        auto json = replay.advance();
        ASSERT_FALSE(json.empty());
        timestamps.push_back(json["timestamp"]);
    }
    
    ASSERT_EQ(8, timestamps.size());
    
    // Verify the pattern repeats (first 4 should match last 4)
    ASSERT_EQ(timestamps[0], timestamps[4]);
    ASSERT_EQ(timestamps[1], timestamps[5]);
    ASSERT_EQ(timestamps[2], timestamps[6]);
    ASSERT_EQ(timestamps[3], timestamps[7]);
}

TEST(loop_toggle) {
    Replay replay("example.csv");
    
    // Start with loop disabled
    ASSERT_FALSE(replay.is_loop_enabled());
    
    // Enable loop
    replay.set_loop(true);
    ASSERT_TRUE(replay.is_loop_enabled());
    
    // Read a couple rows
    auto json1 = replay.advance();
    auto json2 = replay.advance();
    ASSERT_FALSE(json1.empty());
    ASSERT_FALSE(json2.empty());
    
    // Disable loop
    replay.set_loop(false);
    ASSERT_FALSE(replay.is_loop_enabled());
    
    // Continue reading - should stop at end
    int count = 0;
    while (replay.has_next() && count < 10) {
        auto json = replay.advance();
        if (json.empty()) break;
        count++;
    }
    
    ASSERT_EQ(2, count);  // Should read remaining 2 rows and stop
}

// Helper function to run a specific test
bool run_specific_test(const std::string& test_name) {
    if (test_name == "basic_csv_parsing") {
        RUN_TEST(basic_csv_parsing);
    } else if (test_name == "nested_objects") {
        RUN_TEST(nested_objects);
    } else if (test_name == "array_parsing") {
        RUN_TEST(array_parsing);
    } else if (test_name == "multiple_rows") {
        RUN_TEST(multiple_rows);
    } else if (test_name == "reset_functionality") {
        RUN_TEST(reset_functionality);
    } else if (test_name == "comment_line_skipping") {
        RUN_TEST(comment_line_skipping);
    } else if (test_name == "edge_case_comments") {
        RUN_TEST(edge_case_comments);
    } else if (test_name == "play_method_basic") {
        RUN_TEST(play_method_basic);
    } else if (test_name == "play_method_with_filtering") {
        RUN_TEST(play_method_with_filtering);
    } else if (test_name == "play_method_with_reset") {
        RUN_TEST(play_method_with_reset);
    } else if (test_name == "file_not_found") {
        RUN_TEST(file_not_found);
    } else if (test_name == "empty_json_at_end") {
        RUN_TEST(empty_json_at_end);
    } else if (test_name == "type_conversion") {
        RUN_TEST(type_conversion);
    } else if (test_name == "loop_functionality_disabled") {
        RUN_TEST(loop_functionality_disabled);
    } else if (test_name == "loop_functionality_enabled") {
        RUN_TEST(loop_functionality_enabled);
    } else if (test_name == "loop_toggle") {
        RUN_TEST(loop_toggle);
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return false;
    }
    return tests_failed == 0;
}

void run_all_tests() {
  std::cout << "Running Replay class unit tests\n";
  std::cout << "================================\n\n";

  // Run all tests
    RUN_TEST(basic_csv_parsing);
    RUN_TEST(nested_objects);
    RUN_TEST(array_parsing);
    RUN_TEST(multiple_rows);
    RUN_TEST(reset_functionality);
    RUN_TEST(comment_line_skipping);
    RUN_TEST(edge_case_comments);
    RUN_TEST(play_method_basic);
    RUN_TEST(play_method_with_filtering);
    RUN_TEST(play_method_with_reset);
    RUN_TEST(file_not_found);
    RUN_TEST(empty_json_at_end);
    RUN_TEST(type_conversion);
    RUN_TEST(loop_functionality_disabled);
    RUN_TEST(loop_functionality_enabled);
    RUN_TEST(loop_toggle);

  // Print results
  std::cout << "\n================================\n";
  std::cout << "Test Results:\n";
  std::cout << "  Total:  " << tests_total << "\n";
  std::cout << "  Passed: " << tests_passed << "\n";
  std::cout << "  Failed: " << tests_failed << "\n";

  if (tests_failed == 0) {
    std::cout << "\n🎉 All tests PASSED!\n";
  } else {
    std::cout << "\n❌ " << tests_failed << " test(s) FAILED!\n";
  }
}

int main(int argc, char *argv[]) {
  // Check for individual test execution
  if (argc == 3 && std::string(argv[1]) == "--test") {
    std::string test_name = argv[2];
    bool success = run_specific_test(test_name);
    return success ? 0 : 1;
  }

  // Run all tests if no specific test specified
  run_all_tests();
  return tests_failed == 0 ? 0 : 1;
}
