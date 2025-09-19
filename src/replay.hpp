/*
  ____            _
 |  _ \ ___ _ __ | | __ _ _   _
 | |_) / _ \ '_ \| |/ _` | | | |
 |  _ <  __/ |_) | | (_| | |_| |
 |_| \_\___| .__/|_|\__,_|\__, |
           |_|            |___/
Class for replaying CSV files as sequence of JSON objects.
AUthor: Paolo Bosetti, University of Trento
License: MIT
*/

#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class Replay {
public:
  // Constructor takes the path to the CSV file
  explicit Replay(const std::string &csv__filepath)
      : _file(csv__filepath), __headersparsed(false) {
    if (!_file.is_open()) {
      throw std::runtime_error("Failed to open CSV file: " + csv__filepath);
    }
    parse_headers();
  }

  // Destructor to ensure file is closed
  ~Replay() {
    if (_file.is_open()) {
      _file.close();
    }
  }

  // Read the next line and return as JSON object
  // Returns empty JSON object if end of file is reached
  // Skips comment lines (lines starting with '#' with optional leading spaces)
  nlohmann::json advance() {
    std::string line;
    while (std::getline(_file, line)) {
      // Skip comment lines and empty lines
      if (is_comment_line(line) || is_empty_line(line)) {
        continue;
      }

      auto row = parse_csv_line(line);
      return build_json_from_row(row);
    }

    // If we reach EOF and loop mode is enabled, reset and try again
    if (_loop_enabled && _file.eof()) {
      reset();
      // Try to read the first data line after reset
      while (std::getline(_file, line)) {
        // Skip comment lines and empty lines
        if (is_comment_line(line) || is_empty_line(line)) {
          continue;
        }

        auto row = parse_csv_line(line);
        return build_json_from_row(row);
      }
    }

    // Return empty JSON object if no more lines (or loop is disabled)
    return nlohmann::json{};
  }

  // Check if there are more lines to read
  // In loop mode, this always returns true (infinite loop)
  bool has_next() const {
    if (_loop_enabled) {
      return true; // Always has next in loop mode
    }
    return _file.good() && !_file.eof();
  }

  // Reset to beginning of file (after header)
  void reset() {
    _file.clear();
    _file.seekg(0);
    // Skip comment lines and header line
    std::string line;
    while (std::getline(_file, line)) {
      if (is_comment_line(line) || is_empty_line(line)) {
        continue; // Skip comment lines and empty lines
      }
      // This is the header line, we've found it and can stop
      break;
    }
  }

  // Process all remaining lines by calling the provided lambda with each JSON
  // object The lambda should accept a const nlohmann::json& parameter In loop
  // mode, max_cycles limits the number of complete cycles through the data (0 =
  // unlimited cycles, which creates infinite loop) Examples:
  //   replay.play([](const auto& json) { std::cout << json.dump() << std::endl;
  //   }); replay.play([](const auto& json) { process(json); }, 3);  // max 3
  //   cycles
  template <typename Func> void play(Func &&func, size_t max_cycles = 0) {
    if (!_loop_enabled || max_cycles == 0) {
      // Normal mode: process until end of file or unlimited cycles in loop mode
      while (has_next()) {
        auto json_obj = advance();
        if (json_obj.empty()) {
          break;
        }
        func(json_obj);
      }
    } else {
      // Loop mode with cycle limit: first determine rows per cycle
      size_t rows_per_cycle = count_data_rows();
      if (rows_per_cycle == 0) {
        return; // No data to process
      }

      size_t total_rows_to_process = max_cycles * rows_per_cycle;
      size_t rows_processed = 0;

      reset(); // Start from beginning

      while (rows_processed < total_rows_to_process && has_next()) {
        auto json_obj = advance();
        if (json_obj.empty()) {
          break; // Shouldn't happen in loop mode, but safety check
        }

        func(json_obj);
        rows_processed++;
      }
    }
  }

  // Set loop mode - if true, advance() will reset to beginning when EOF is
  // reached
  void set_loop(bool enabled) { _loop_enabled = enabled; }

  // Get current loop mode
  bool is_loop_enabled() const { return _loop_enabled; }

private:
  std::ifstream _file;
  std::vector<nlohmann::json::json_pointer> _headers;
  bool __headersparsed;
  bool _loop_enabled = false; // Loop mode flag

  // Helper methods

  // Count the number of data rows in the file (excluding header and comments)
  size_t count_data_rows() {
    // Save current position
    auto current_pos = _file.tellg();

    // Reset to beginning to count rows
    reset();

    size_t count = 0;
    std::string line;
    while (std::getline(_file, line)) {
      // Skip comment lines and empty lines
      if (is_comment_line(line) || is_empty_line(line)) {
        continue;
      }
      count++;
    }

    // Restore file position
    _file.clear();
    _file.seekg(current_pos);

    return count;
  }

  void parse_headers() {
    std::string header_line;
    while (std::getline(_file, header_line)) {
      // Skip comment lines and empty lines to find the actual header
      if (is_comment_line(header_line) || is_empty_line(header_line)) {
        continue;
      }
      std::vector<std::string> keypaths = parse_csv_line(header_line);
      for (auto &kp : keypaths) {
        _headers.emplace_back(pointer_from_string(kp));
      }
      __headersparsed = true;
      return;
    }

    throw std::runtime_error("CSV file is empty or cannot read header line");
  }

  std::vector<std::string> parse_csv_line(const std::string &line) {
    std::vector<std::string> result;
    std::stringstream ss;
    bool in_quotes = false;

    for (size_t i = 0; i < line.length(); ++i) {
      char c = line[i];

      if (c == '"') {
        in_quotes = !in_quotes;
      } else if (c == ',' && !in_quotes) {
        result.push_back(ss.str());
        ss.str("");
        ss.clear();
      } else {
        ss << c;
      }
    }

    // Add the last field
    result.push_back(ss.str());

    return result;
  }

  std::string normalize_keypath(const std::string &input) {
    std::string output;
    if (input[0] == '/') { // already a json_pointer string
      return input;
    }
    output.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
      char c = input[i];

      if (c == ']') {
        // If ']' is followed by '.', replace the pair "]." with '/'
        if (i + 1 < input.size() && input[i + 1] == '.') {
          output.push_back('/');
          ++i; // skip the dot as well
        }
        // otherwise skip the ']' (remove it)
      } else if (c == '.' || c == '[') {
        output.push_back('/');
      } else {
        output.push_back(c);
      }
    }

    return output;
  }

  nlohmann::json::json_pointer pointer_from_string(std::string &path) {
    nlohmann::json::json_pointer ptr(normalize_keypath(path));
    return ptr;
  }

  nlohmann::json build_json_from_row(const std::vector<std::string> &row) {
    nlohmann::json result = nlohmann::json::object();
    for (size_t i = 0; i < _headers.size() && i < row.size(); ++i) {
      const nlohmann::json::json_pointer &header = _headers[i];
      const std::string &value = row[i];
      if (is_numeric(value)) {
        result[header] = parse_number(value);
      } else {
        result[header] = value;
      }
    }
    return result;
  }


  bool is_numeric(const std::string &str) {
    if (str.empty())
      return false;

    char *end;
    std::strtod(str.c_str(), &end);
    return end == str.c_str() + str.length();
  }

  double parse_number(const std::string &str) {
    return std::strtod(str.c_str(), nullptr);
  }

  // Check if a line is a comment (starts with '#' with optional leading spaces)
  bool is_comment_line(const std::string &line) {
    // Find first non-space character
    size_t first_char = line.find_first_not_of(' ');
    if (first_char == std::string::npos) {
      return false; // Empty line or only spaces - not a comment
    }
    return line[first_char] == '#';
  }

  // Check if a line is empty or contains only whitespace
  bool is_empty_line(const std::string &line) {
    return line.find_first_not_of(" \t\r\n") == std::string::npos;
  }
};
