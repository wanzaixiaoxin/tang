#include <tang/tang.h>
#include <iostream>
#include <thread>
#include <vector>

// Sender coroutine function
void sender(tang::channel<int>& ch, int id, int count) {
    for (int i = 0; i < count; ++i) {
        int value = id * 100 + i;
        
        // Use send operator
        ch << value;
        
        std::cout << "Sender " << id << " sent: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Receiver coroutine function
void receiver(tang::channel<int>& ch, int id, int count) {
    for (int i = 0; i < count; ++i) {
        int value;
        
        // Use receive operator
        ch >> value;
        
        std::cout << "Receiver " << id << " received: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

// Buffered channel example
void buffered_channel_example() {
    std::cout << "\n=== Buffered Channel Example ===" << std::endl;
    
    tang::channel<int> ch(5);
    
    tang::go(sender, ch, 1, 10);
    tang::go(sender, ch, 2, 10);
    
    tang::go(receiver, ch, 1, 10);
    tang::go(receiver, ch, 2, 10);
}

// Unbuffered channel example
void unbuffered_channel_example() {
    std::cout << "\n=== Unbuffered Channel Example ===" << std::endl;
    
    // Create an unbuffered channel
    tang::channel<std::string> ch;
    
    // Start sender coroutine
    tang::go([&ch]() {
        std::vector<std::string> messages = {"Hello", "from", "unbuffered", "channel"};
        
        for (const auto& msg : messages) {
            ch << msg;
            std::cout << "Sent: " << msg << std::endl;
        }
        
        // Close channel
        ch.close();
    });
    
    // Start receiver coroutine
    tang::go([&ch]() {
        std::string msg;
        
        // Receive data from channel until it's closed
        while (ch >> msg) {
            std::cout << "Received: " << msg << std::endl;
        }
        
        std::cout << "Channel closed" << std::endl;
    });
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // Initialize runtime with 4 worker threads
    tang::runtime::init(4);
    
    // Run buffered channel example
    buffered_channel_example();
    
    // Run unbuffered channel example
    unbuffered_channel_example();
    
    // Run scheduler, block until all coroutines complete
    tang::runtime::run();
    
    std::cout << "\nAll examples completed!" << std::endl;
    
    return 0;
}
