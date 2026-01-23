#include <tang/tang.h>
#include <iostream>
#include <thread>

// Coroutine function to send data to channel
void send_data(tang::channel<int>& ch, int id, int delay_ms, int count) {
    for (int i = 0; i < count; ++i) {
        int value = id * 100 + i;
        
        // Use send operator
        ch << value;
        
        std::cout << "Sender " << id << " sent: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // Simulate work delay
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
    
    // Close channel
    ch.close();
}

// Coroutine function to receive data from channels using select
void select_example() {
    std::cout << "\n=== Select Example ===" << std::endl;
    
    // Create channels for three senders
    tang::channel<int> ch1(5);
    tang::channel<int> ch2(5);
    tang::channel<int> ch3(5);
    
    // Start three senders with different send rates
    tang::go(send_data, ch1, 1, 100, 5);  // Send every 100ms, total 5 times
    tang::go(send_data, ch2, 2, 200, 5);  // Send every 200ms, total 5 times
    tang::go(send_data, ch3, 3, 300, 5);  // Send every 300ms, total 5 times
    
    // Receive counter for three senders
    int received = 0;
    const int total = 15;
    
    // Use receive data using select
    while (received < total) {
        int value;
        
        // Use select to wait for multiple channels
        tang::select(
            // Receive case 1   
            tang::case_recv(ch1, value, [&]() {
                std::cout << "Select received from ch1: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            }),
            
            // Receive case 2   
            tang::case_recv(ch2, value, [&]() {
                std::cout << "Select received from ch2: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            }),
            
            // Receive case 3   
            tang::case_recv(ch3, value, [&]() {
                std::cout << "Select received from ch3: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            })
        );
    }
    
    std::cout << "Select example completed!" << std::endl;
}

// Coroutine function to receive data from channel with default case using select
void select_with_default_example() {
    std::cout << "\n=== Select with Default Case Example ===" << std::endl;
    
    // Create channel for one sender
    tang::channel<int> ch;
    
    // Start one sender coroutine with delay send
    tang::go([&ch]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ch << 42;
        std::cout << "Sender sent: 42" << std::endl;
        ch.close();
    });
    
    // Receive counter for one sender
    int received_count = 0;
    const int max_attempts = 10;
    
    for (int i = 0; i < max_attempts; ++i) {
        int value;
        bool has_value = false;
        
        tang::select(
            // Receive case   
            tang::case_recv(ch, value, [&]() {
                std::cout << "Select received: " << value << std::endl;
                received_count++;
                has_value = true;
            }),
            
            // Default case   
            tang::default_case([&]() {
                std::cout << "Select default case executed" << std::endl;
            })
        );
        
        if (has_value) {
            break;
        }
        
        // Short sleep to reduce CPU usage when no data received
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Select with default example completed! Received " << received_count << " values" << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // Initialize runtime with 4 worker threads
    tang::runtime::init(4);
    
    // Run select example
    select_example();
    
    // Run select with default case example
    select_with_default_example();
    
    // Run scheduler, block until all coroutines are completed
    tang::runtime::run();
    
    std::cout << "\nAll select examples completed!" << std::endl;
    
    return 0;
}
