#include "../src/replay.hpp"
#include <iostream>
#include <vector>

int main() {
    try {
        std::cout << "Testing loop functionality:\n";
        std::cout << "==========================\n\n";
        
        // Test 1: Normal mode (no loop) - should read 4 rows and stop
        std::cout << "Test 1: Normal mode (loop disabled)\n";
        std::cout << "-----------------------------------\n";
        
        Replay replay1("example.csv");
        std::vector<double> timestamps_normal;
        
        int count = 0;
        while (replay1.has_next() && count < 10) {  // Safety limit
            auto json = replay1.advance();
            if (json.empty()) break;
            
            timestamps_normal.push_back(json["timestamp"]);
            count++;
            std::cout << "Row " << count << ": timestamp=" << json["timestamp"] << "\n";
        }
        
        std::cout << "Total rows read in normal mode: " << count << "\n";
        std::cout << "Expected: 4 (should stop after reading all data)\n\n";
        
        // Test 2: Loop mode - should cycle back to beginning
        std::cout << "Test 2: Loop mode (loop enabled)\n";
        std::cout << "--------------------------------\n";
        
        Replay replay2("example.csv");
        replay2.set_loop(true);  // Enable loop mode
        
        std::vector<double> timestamps_loop;
        count = 0;
        int max_iterations = 10;  // Read more than the 4 available rows
        
        while (count < max_iterations) {
            auto json = replay2.advance();
            if (json.empty()) {
                std::cout << "Unexpected: got empty JSON in loop mode\n";
                break;
            }
            
            timestamps_loop.push_back(json["timestamp"]);
            count++;
            std::cout << "Row " << count << ": timestamp=" << json["timestamp"] << "\n";
        }
        
        std::cout << "Total rows read in loop mode: " << count << "\n";
        std::cout << "Expected: 10 (should loop through the 4 data rows multiple times)\n\n";
        
        // Verify looping behavior
        std::cout << "Verifying loop behavior:\n";
        std::cout << "-----------------------\n";
        
        bool loops_correctly = true;
        for (int i = 0; i < count; i++) {
            int expected_index = i % 4;  // Should cycle through 0,1,2,3,0,1,2,3...
            double expected_timestamp = timestamps_normal[expected_index];
            
            if (timestamps_loop[i] != expected_timestamp) {
                std::cout << "❌ Mismatch at position " << i 
                          << ": got " << timestamps_loop[i] 
                          << ", expected " << expected_timestamp << "\n";
                loops_correctly = false;
            }
        }
        
        if (loops_correctly) {
            std::cout << "✅ Loop behavior is correct!\n";
            std::cout << "   Pattern: " << timestamps_loop[0] << " → " << timestamps_loop[1] 
                      << " → " << timestamps_loop[2] << " → " << timestamps_loop[3] 
                      << " → " << timestamps_loop[4] << " → " << timestamps_loop[5] << " ...\n";
        }
        
        // Test 3: Verify loop can be disabled
        std::cout << "\nTest 3: Disabling loop mode\n";
        std::cout << "---------------------------\n";
        
        Replay replay3("example.csv");
        replay3.set_loop(true);   // Enable loop
        
        // Read a few rows
        auto json1 = replay3.advance();
        auto json2 = replay3.advance();
        std::cout << "Read 2 rows in loop mode\n";
        
        replay3.set_loop(false);  // Disable loop
        std::cout << "Loop mode disabled\n";
        
        // Continue reading until end
        count = 0;
        while (replay3.has_next() && count < 10) {
            auto json = replay3.advance();
            if (json.empty()) break;
            count++;
        }
        
        std::cout << "Remaining rows after disabling loop: " << count << "\n";
        std::cout << "Expected: 2 (should stop at end of file)\n";
        
        // Test 4: is_loop_enabled() method
        std::cout << "\nTest 4: Loop status checking\n";
        std::cout << "----------------------------\n";
        
        Replay replay4("example.csv");
        std::cout << "Initially loop enabled: " << (replay4.is_loop_enabled() ? "true" : "false") << "\n";
        
        replay4.set_loop(true);
        std::cout << "After set_loop(true): " << (replay4.is_loop_enabled() ? "true" : "false") << "\n";
        
        replay4.set_loop(false);
        std::cout << "After set_loop(false): " << (replay4.is_loop_enabled() ? "true" : "false") << "\n";
        
        std::cout << "\n🎉 All loop functionality tests completed!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
