# Replay CSV to JSON Converter

A **header-only** C++17 class that reads CSV files line by line and converts them to `nlohmann::json` objects with support for nested keypaths and arrays.

## Features

- **Header-Only**: Single header file - just `#include "replay.hpp"` and you're ready to go!
- **Lambda Support**: `play()` method for functional-style processing with lambdas
- **Comment Support**: Lines starting with `#` (with optional leading spaces) are automatically skipped
- **Nested Objects**: Column names with dots (e.g., `acceleration.x`) create nested JSON objects
- **Arrays**: Column names with numeric indices (e.g., `signal[0]`, `signal[1]`, `signal[2]`) create JSON arrays
- **Type Detection**: Automatically converts numeric values to JSON numbers
- **CSV Parsing**: Handles quoted fields and commas within fields
- **File Navigation**: Support for reading line by line and resetting to the beginning

## Example

Given this CSV file:
```csv
# This is a comment - it will be skipped
timestamp,acceleration.x,acceleration.y,signal[0],signal[1],signal[2],driver.name
# Another comment with data explanation
1609459200,2.5,1.3,101,102,103,John Doe
  # Comments can have leading spaces too
```

The `Replay::advance()` method returns:
```json
{
  "timestamp": 1609459200,
  "acceleration": {
    "x": 2.5,
    "y": 1.3
  },
  "signal": [101, 102, 103],
  "driver": {
    "name": "John Doe"
  }
}
```

## Building

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- nlohmann/json library (automatically downloaded if not found)

### Build Targets

The CMakeLists.txt provides several useful targets:

- `make` - Build all executables
- `make test_replay` - Build unit test executable  
- `ctest` - **Run individual unit tests using CTest framework** ⭐
- `make run_tests` - Run legacy combined unit test
- `make run_examples` - Run all example programs
- `make validate` - Run full validation suite

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

The project uses **CTest** for comprehensive unit testing with individual test cases:

```bash
# Run all tests (14 individual test cases)
ctest

# Run specific tests by pattern
ctest -R "PlayMethod"          # Run all play method tests
ctest -R "BasicCSV|Array"      # Run basic CSV and array tests

# Run tests with verbose output
ctest --verbose

# Run tests in parallel
ctest -j4

# Run legacy combined test executable
make run_tests

# Run all example programs
make run_examples

# Run full validation suite (examples + tests)
make validate
```

### Alternative: Install nlohmann/json manually

If you prefer to install nlohmann/json yourself:

**On macOS with Homebrew:**
```bash
brew install nlohmann-json
```

**On Ubuntu/Debian:**
```bash
sudo apt-get install nlohmann-json3-dev
```

## Usage

Since it's header-only, usage is simple - just include the header:

### Method 1: Using the play() method (recommended)
```cpp
#include "replay.hpp"  // That's it! No linking required.

int main() {
    try {
        Replay replay("data.csv");
        
        // Process all rows with a lambda - simple and clean!
        replay.play([](const auto& json) {
            std::cout << "Speed: " << json["speed"] << " km/h\n";
        });
        
        // Reset and do data aggregation
        replay.reset();
        double max_speed = 0;
        replay.play([&max_speed](const auto& json) {
            max_speed = std::max(max_speed, static_cast<double>(json["speed"]));
        });
        std::cout << "Max speed: " << max_speed << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

### Method 2: Manual iteration
```cpp
#include "replay.hpp"

int main() {
    try {
        Replay replay("data.csv");
        
        while (replay.has_next()) {
            auto json_obj = replay.advance();
            if (json_obj.empty()) break;
            
            std::cout << json_obj.dump(2) << std::endl;
        }
        
        // Reset to beginning if needed
        replay.reset();
        auto first_line = replay.advance();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## API Reference

### Constructor
```cpp
Replay(const std::string& csv_file_path)
```
Opens the CSV file and parses the header row.

### Methods

#### `nlohmann::json advance()`
Reads the next line from the CSV file and returns it as a JSON object. Returns an empty JSON object if end of file is reached.

#### `bool has_next() const`
Returns `true` if there are more lines to read.

#### `void reset()`
Resets the file pointer to the beginning (after the header row).

#### `template<typename Func> void play(Func&& func)`
Processes all remaining lines in the CSV file by calling the provided lambda/function with each JSON object. This is the recommended way to process data as it's more concise and functional in style.

**Example:**
```cpp
replay.play([](const auto& json) {
    std::cout << json["field_name"] << std::endl;
});
```

## CSV File Format

### Comments and Empty Lines
- Lines starting with `#` (with optional leading spaces) are treated as comments and skipped
- Empty lines and lines with only whitespace are automatically skipped
- Comments can appear anywhere in the file (before header, between data rows, etc.)

### Column Naming Rules
1. **Simple fields**: `name` → `{"name": "value"}`
2. **Nested objects**: `position.latitude` → `{"position": {"latitude": value}}`
3. **Arrays**: `signal.0`, `signal.1`, `signal.2` → `{"signal": [val0, val1, val2]}`
4. **Deep nesting**: `vehicle.engine.rpm` → `{"vehicle": {"engine": {"rpm": value}}}`

## Running the Demo

After building, run the demo:
```bash
./replay_demo
```

This will process the included `example.csv` file and display the JSON output.

## Unit Tests

The project includes **14 individual CTest test cases** in `test_replay.cpp`:

- ✅ **BasicCSVParsing** - File loading and JSON conversion
- ✅ **NestedObjects** - Keypath parsing (`acceleration.x`)
- ✅ **ArrayParsing** - Numeric indices (`signal.0`, `signal.1`)
- ✅ **CommentLineSkipping** - Lines starting with `#`
- ✅ **PlayMethodBasic** - Lambda processing functionality
- ✅ **PlayMethodWithFiltering** - Conditional lambda processing
- ✅ **PlayMethodWithReset** - Reset + play combinations
- ✅ **ResetFunctionality** - File position management
- ✅ **FileNotFound** - Error handling for missing files
- ✅ **TypeConversion** - Numbers vs strings vs arrays
- ✅ **EdgeCaseComments** - Empty lines, mixed comments
- ✅ **EmptyJSONAtEnd** - End-of-file handling
- ✅ **MultipleRows** - Processing multiple CSV lines
- ✅ **AllTests** - Combined test execution

**Run individual tests:** `ctest -R "TestName"` | **Run all:** `ctest`

## Project Structure

```
replay/
├── replay.hpp              # Header-only implementation (~280 lines)
├── example.csv             # Test data without comments
├── example_with_comments.csv # Test data with comments
├── edge_case_comments.csv  # Edge case test data
├── main.cpp               # Full demo program (manual iteration)
├── simple_example.cpp     # Simple usage example
├── play_example.cpp       # Play method usage examples
├── test_comments.cpp      # Comment functionality test
├── test_play_method.cpp   # Play method comprehensive test
├── test_replay.cpp        # Unit test suite (13 tests)
├── CMakeLists.txt         # Build configuration with CTest
└── README.md              # Documentation
```
