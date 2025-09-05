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
      return true;  // Always has next in loop mode
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

  // Process all remaining lines by calling the provided lambda with each JSON object
  // The lambda should accept a const nlohmann::json& parameter
  // In loop mode, max_cycles limits the number of complete cycles through the data
  // (0 = unlimited cycles, which creates infinite loop)
  // Examples:
  //   replay.play([](const auto& json) { std::cout << json.dump() << std::endl; });
  //   replay.play([](const auto& json) { process(json); }, 3);  // max 3 cycles
  template <typename Func> 
  void play(Func &&func, size_t max_cycles = 0) {
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
          break;  // Shouldn't happen in loop mode, but safety check
        }
        
        func(json_obj);
        rows_processed++;
      }
    }
  }

  // Set loop mode - if true, advance() will reset to beginning when EOF is reached
  void set_loop(bool enabled) {
    _loop_enabled = enabled;
  }
  
  // Get current loop mode
  bool is_loop_enabled() const {
    return _loop_enabled;
  }

private:
  std::ifstream _file;
  std::vector<std::string> _headers;
  bool __headersparsed;
  bool _loop_enabled = false;  // Loop mode flag

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

      _headers = parse_csv_line(header_line);
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

  nlohmann::json build_json_from_row(const std::vector<std::string> &row) {
    nlohmann::json result = nlohmann::json::object();

    // Group array indices by their base keypath
    std::map<std::string, std::map<int, std::string>> arrays;

    for (size_t i = 0; i < _headers.size() && i < row.size(); ++i) {
      const std::string &header = _headers[i];
      const std::string &value = row[i];

      // Check if this is an array index (contains a number in the keypath)
      auto parts = split_keypath(header);
      bool is_array = false;
      std::string base_keypath;
      int array_index = -1;

      for (size_t j = 0; j < parts.size(); ++j) {
        if (is_numeric(parts[j])) {
          is_array = true;
          array_index = static_cast<int>(std::stoi(parts[j]));

          // Build base keypath (everything before the numeric part)
          for (size_t k = 0; k < j; ++k) {
            if (k > 0)
              base_keypath += ".";
            base_keypath += parts[k];
          }
          break;
        }
      }

      if (is_array) {
        arrays[base_keypath][array_index] = value;
      } else {
        set_nested_value(result, header, value);
      }
    }

    // Process arrays
    for (const auto &array_pair : arrays) {
      const std::string &base_path = array_pair.first;
      const auto &index_value_map = array_pair.second;

      // Find the maximum index to size the array properly
      int max_index = -1;
      for (const auto &iv : index_value_map) {
        max_index = std::max(max_index, iv.first);
      }

      // Create array
      nlohmann::json array = nlohmann::json::array();
      for (int i = 0; i <= max_index; ++i) {
        auto it = index_value_map.find(i);
        if (it != index_value_map.end()) {
          // Try to parse as number first, then as string
          if (is_numeric(it->second)) {
            array.push_back(parse_number(it->second));
          } else {
            array.push_back(it->second);
          }
        } else {
          array.push_back(nullptr); // Missing index
        }
      }

      set_nested_value(result, base_path, array);
    }

    return result;
  }

  void set_nested_value(nlohmann::json &obj, const std::string &keypath,
                        const std::string &value) {
    auto parts = split_keypath(keypath);
    nlohmann::json *current = &obj;

    for (size_t i = 0; i < parts.size() - 1; ++i) {
      if (!current->contains(parts[i]) || !(*current)[parts[i]].is_object()) {
        (*current)[parts[i]] = nlohmann::json::object();
      }
      current = &(*current)[parts[i]];
    }

    // Set the final value, trying to parse as number first
    if (is_numeric(value)) {
      (*current)[parts.back()] = parse_number(value);
    } else {
      (*current)[parts.back()] = value;
    }
  }

  void set_nested_value(nlohmann::json &obj, const std::string &keypath,
                        const nlohmann::json &value) {
    auto parts = split_keypath(keypath);
    nlohmann::json *current = &obj;

    for (size_t i = 0; i < parts.size() - 1; ++i) {
      if (!current->contains(parts[i]) || !(*current)[parts[i]].is_object()) {
        (*current)[parts[i]] = nlohmann::json::object();
      }
      current = &(*current)[parts[i]];
    }

    (*current)[parts.back()] = value;
  }

  std::vector<std::string> split_keypath(const std::string &keypath) {
    std::vector<std::string> parts;
    std::stringstream ss(keypath);
    std::string part;

    while (std::getline(ss, part, '.')) {
      parts.push_back(part);
    }

    return parts;
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
