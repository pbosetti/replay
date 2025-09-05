cmake_minimum_required(VERSION 3.10)
project(ReplayCSV VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REPLAY_BUILD_EXAMPLES "Build example executables" OFF)
set(REPLAY_BUILD_EXAMPLES ${REPLAY_BUILD_EXAMPLES} CACHE BOOL "Build example executables")
option(REPLAY_BUILD_TESTS "Build test executables" OFF)
set(REPLAY_BUILD_TESTS ${REPLAY_BUILD_TESTS} CACHE BOOL "Build test executables")

# Add compiler flags for better debugging and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find nlohmann/json
find_package(nlohmann_json QUIET)

# If nlohmann/json is not found via find_package, try using FetchContent
if(NOT TARGET nlohmann_json::nlohmann_json)
  include(FetchContent)

  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
  )

  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(${REPLAY_BUILD_EXAMPLES})
  message(STATUS "Building example executables")

  # ============================================================================
  # EXECUTABLES
  # ============================================================================

  set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
  set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
  set(EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/example)
  include_directories(${SRC_DIR})

  # Main demo executable
  add_executable(replay_demo ${EXAMPLE_DIR}/main.cpp)
  target_link_libraries(replay_demo PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(replay_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Simple example executable
  add_executable(simple_example ${EXAMPLE_DIR}/simple_example.cpp)
  target_link_libraries(simple_example PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(simple_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Play method example executable
  add_executable(play_example ${EXAMPLE_DIR}/play_example.cpp)
  target_link_libraries(play_example PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(play_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Play method comprehensive test executable
  add_executable(test_play_method ${TEST_DIR}/test_play_method.cpp)
  target_link_libraries(test_play_method PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(test_play_method PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Loop functionality test executable
  add_executable(test_loop_functionality ${TEST_DIR}/test_loop_functionality.cpp)
  target_link_libraries(test_loop_functionality PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(test_loop_functionality PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

else()
  message(STATUS "Not building example executables (set REPLAY_BUILD_EXAMPLES=ON to enable)")
endif()


if(${REPLAY_BUILD_TESTS} AND ${REPLAY_BUILD_EXAMPLES})
  message(STATUS "Building test executables")

  # Enable testing
  enable_testing()

  # ============================================================================
  # UNIT TESTS
  # ============================================================================

  # Main unit test executable
  add_executable(test_replay ${TEST_DIR}/test_replay.cpp)
  target_link_libraries(test_replay PRIVATE nlohmann_json::nlohmann_json)
  target_include_directories(test_replay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Individual CTest test cases
  add_test(NAME BasicCSVParsing COMMAND test_replay --test basic_csv_parsing)
  add_test(NAME NestedObjects COMMAND test_replay --test nested_objects)
  add_test(NAME ArrayParsing COMMAND test_replay --test array_parsing)
  add_test(NAME MultipleRows COMMAND test_replay --test multiple_rows)
  add_test(NAME ResetFunctionality COMMAND test_replay --test reset_functionality)
  add_test(NAME CommentLineSkipping COMMAND test_replay --test comment_line_skipping)
  add_test(NAME EdgeCaseComments COMMAND test_replay --test edge_case_comments)
  add_test(NAME PlayMethodBasic COMMAND test_replay --test play_method_basic)
  add_test(NAME PlayMethodWithFiltering COMMAND test_replay --test play_method_with_filtering)
  add_test(NAME PlayMethodWithReset COMMAND test_replay --test play_method_with_reset)
  add_test(NAME FileNotFound COMMAND test_replay --test file_not_found)
  add_test(NAME EmptyJSONAtEnd COMMAND test_replay --test empty_json_at_end)
  add_test(NAME TypeConversion COMMAND test_replay --test type_conversion)
  add_test(NAME LoopFunctionalityDisabled COMMAND test_replay --test loop_functionality_disabled)
  add_test(NAME LoopFunctionalityEnabled COMMAND test_replay --test loop_functionality_enabled)
  add_test(NAME LoopToggle COMMAND test_replay --test loop_toggle)

  # Also add the comprehensive test that runs all at once
  add_test(NAME AllTests COMMAND test_replay)

  # Set working directory for all tests so they can find CSV files
  set_tests_properties(
    BasicCSVParsing NestedObjects ArrayParsing MultipleRows ResetFunctionality
    CommentLineSkipping EdgeCaseComments PlayMethodBasic PlayMethodWithFiltering
    PlayMethodWithReset FileNotFound EmptyJSONAtEnd TypeConversion
    LoopFunctionalityDisabled LoopFunctionalityEnabled LoopToggle AllTests
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

endif()
