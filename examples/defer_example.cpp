#include <tang/tang.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>

void simple_defer() {
    std::cout << "Entering simple_defer()" << std::endl;
    
    defer {
        std::cout << "Exiting simple_defer() - defer executed" << std::endl;
    } end_defer
    
    std::cout << "Doing work in simple_defer()" << std::endl;
}

void multiple_defer() {
    std::cout << "\nEntering multiple_defer()" << std::endl;
    
    defer {
        std::cout << "Exiting multiple_defer() - defer 1 executed" << std::endl;
    } end_defer
    
    defer {
        std::cout << "Exiting multiple_defer() - defer 2 executed" << std::endl;
    } end_defer
    
    defer {
        std::cout << "Exiting multiple_defer() - defer 3 executed" << std::endl;
    } end_defer
    
    std::cout << "Doing work in multiple_defer()" << std::endl;
}

void file_defer_example() {
    std::cout << "\nEntering file_defer_example()" << std::endl;
    
    std::ofstream file("test.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }
    
    defer {
        if (file.is_open()) {
            file.close();
            std::cout << "File closed by defer" << std::endl;
        }
    } end_defer
    
    file << "Hello, Tang!" << std::endl;
    file << "This is a test file." << std::endl;
    
    std::cout << "File operations completed" << std::endl;
}

void mutex_defer_example() {
    std::cout << "\nEntering mutex_defer_example()" << std::endl;
    
    static std::mutex mtx;
    static std::vector<int> shared_data;
    
    mtx.lock();
    
    defer {
        mtx.unlock();
        std::cout << "Mutex unlocked by defer" << std::endl;
    } end_defer
    
    shared_data.push_back(42);
    shared_data.push_back(100);
    shared_data.push_back(200);
    
    std::cout << "Shared data modified. Size: " << shared_data.size() << std::endl;
}

void exception_defer_example() {
    std::cout << "\nEntering exception_defer_example()" << std::endl;
    
    try {
        defer {
            std::cout << "Exception defer executed" << std::endl;
        } end_defer
        
        std::cout << "Doing work before exception" << std::endl;
        
        throw std::runtime_error("Test exception");

    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    
    std::cout << "Exiting exception_defer_example()" << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    tang::runtime::init(2);
    
    simple_defer();
    multiple_defer();
    file_defer_example();
    mutex_defer_example();
    exception_defer_example();
    
    tang::go([]() {
        std::cout << "\nEntering goroutine" << std::endl;
        
        defer {
            std::cout << "Goroutine defer executed" << std::endl;
        } end_defer
        
        std::cout << "Doing work in goroutine" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Goroutine work completed" << std::endl;
    });
    
    tang::runtime::run();
    
    std::cout << "\nAll defer examples completed!" << std::endl;
    
    return 0;
}
