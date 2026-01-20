#include <tang/tang.h>
#include <iostream>
#include <thread>

void hello() {
    std::cout << "Hello from goroutine! Thread ID: " << std::this_thread::get_id() << std::endl;
}

void hello_with_name(const std::string& name) {
    std::cout << "Hello, " << name << "! Thread ID: " << std::this_thread::get_id() << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    tang::runtime::init();
    
    for (int i = 0; i < 10; ++i) {
        tang::go(hello);
        
        tang::spawn(hello_with_name, "Tang");
        
        tang::go([i]() {
            std::cout << "Hello from lambda goroutine " << i << "! Thread ID: " << std::this_thread::get_id() << std::endl;
        });
    }
    
    tang::runtime::run();
    
    std::cout << "All goroutines completed!" << std::endl;
    
    return 0;
}
