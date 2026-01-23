#include "test_framework.h"

namespace tang {
namespace test {

// Global test framework instance
TestFramework& get_test_framework() {
    static TestFramework framework;
    return framework;
}

// Main function for running tests
int run_tests(int argc, char* argv[]) {
    try {
        LOG_INFO(tang::logger::test, "Starting test framework...");
        
        bool success = get_test_framework().run_all();
        
        if (success) {
            LOG_INFO(tang::logger::test, "All tests completed successfully!");
            return 0;
        } else {
            LOG_ERROR(tang::logger::test, "Some tests failed!");
            return 1;
        }
    } catch (const std::exception& e) {
        LOG_ERROR(tang::logger::test, std::string("Test framework error: ") + e.what());
        return 1;
    }
}

} // namespace test
} // namespace tang