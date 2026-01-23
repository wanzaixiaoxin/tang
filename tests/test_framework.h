#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "tang/logger.h"
#include "tang/runtime.h"

namespace tang {
namespace test {

// Runtime management helpers
class RuntimeScope {
private:
    static std::unique_ptr<tang::RuntimeScope> global_runtime_;
    
public:
    /**
     * @brief Initialize the global runtime
     * 
     * @param threads Number of worker threads
     */
    static void init(size_t threads = 2) {
        if (!global_runtime_) {
            global_runtime_ = std::make_unique<tang::RuntimeScope>(threads);
        }
    }
    
    /**
     * @brief Run the global runtime
     */
    static void run() {
        if (global_runtime_) {
            global_runtime_->run();
        }
    }
    
    /**
     * @brief Cleanup the global runtime
     */
    static void cleanup() {
        global_runtime_.reset();
    }
};

// Global runtime instance
inline std::unique_ptr<tang::RuntimeScope> RuntimeScope::global_runtime_ = nullptr;

/**
 * Test case result
 */
struct TestResult {
    std::string name;
    bool passed;
    std::string error_message;
    std::chrono::duration<double> duration;
    
    TestResult(const std::string& test_name, bool success, const std::string& msg = "", 
               std::chrono::duration<double> time = std::chrono::duration<double>::zero())
        : name(test_name), passed(success), error_message(msg), duration(time) {}
};

/**
 * Test case function type
 */
using TestFunction = std::function<void()>;

/**
 * Test case definition
 */
struct TestCase {
    std::string name;
    TestFunction function;
    
    TestCase(const std::string& test_name, TestFunction test_func)
        : name(test_name), function(test_func) {}
};

/**
 * Test framework class
 */
class TestFramework {
private:
    std::vector<TestCase> test_cases_;
    std::vector<TestResult> results_;
    std::atomic<int> passed_count_{0};
    std::atomic<int> failed_count_{0};
    
    /**
     * Run a single test case
     */
    TestResult run_test_case(const TestCase& test_case) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            LOG_INFO(tang::logger::test, "Running test: " + test_case.name);
            test_case.function();
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = end_time - start_time;
            
            LOG_INFO(tang::logger::test, "Test passed: " + test_case.name);
            return TestResult(test_case.name, true, "", duration);
        } catch (const std::exception& e) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = end_time - start_time;
            
            std::string error_msg = "Test failed: " + std::string(e.what());
            LOG_ERROR(tang::logger::test, error_msg);
            return TestResult(test_case.name, false, error_msg, duration);
        } catch (...) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = end_time - start_time;
            
            std::string error_msg = "Test failed with unknown exception";
            LOG_ERROR(tang::logger::test, error_msg);
            return TestResult(test_case.name, false, error_msg, duration);
        }
    }

public:
    /**
     * Add a test case to the framework
     */
    void add_test(const std::string& name, TestFunction function) {
        test_cases_.emplace_back(name, function);
    }
    
    /**
     * Run all test cases
     */
    bool run_all() {
        LOG_INFO(tang::logger::test, "Starting test suite with " + std::to_string(test_cases_.size()) + " tests");
        
        passed_count_ = 0;
        failed_count_ = 0;
        results_.clear();
        
        for (const auto& test_case : test_cases_) {
            TestResult result = run_test_case(test_case);
            results_.push_back(result);
            
            if (result.passed) {
                passed_count_++;
            } else {
                failed_count_++;
            }
        }
        
        return report_results();
    }
    
    /**
     * Report test results
     */
    bool report_results() {
        LOG_INFO(tang::logger::test, "\n=== Test Results ===");
        LOG_INFO(tang::logger::test, "Total tests: " + std::to_string(test_cases_.size()));
        LOG_INFO(tang::logger::test, "Passed: " + std::to_string(passed_count_));
        LOG_INFO(tang::logger::test, "Failed: " + std::to_string(failed_count_));
        
        if (failed_count_ > 0) {
            LOG_INFO(tang::logger::test, "\nFailed tests:");
            for (const auto& result : results_) {
                if (!result.passed) {
                    LOG_ERROR(tang::logger::test, "- " + result.name + ": " + result.error_message);
                }
            }
        }
        
        LOG_INFO(tang::logger::test, "\nDetailed results:");
        for (const auto& result : results_) {
            std::string status = result.passed ? "PASS" : "FAIL";
            std::stringstream duration_ss;
            duration_ss << std::fixed << std::setprecision(3) << result.duration.count() << "s";
            
            LOG_INFO(tang::logger::test, "[" + status + "] " + result.name + " (" + duration_ss.str() + ")");
        }
        
        bool all_passed = (failed_count_ == 0);
        if (all_passed) {
            LOG_INFO(tang::logger::test, "\nAll tests passed!");
        } else {
            LOG_ERROR(tang::logger::test, "\nSome tests failed!");
        }
        
        return all_passed;
    }
    
    /**
     * Get test results
     */
    const std::vector<TestResult>& get_results() const {
        return results_;
    }
    
    /**
     * Get passed count
     */
    int get_passed_count() const {
        return passed_count_;
    }
    
    /**
     * Get failed count
     */
    int get_failed_count() const {
        return failed_count_;
    }
};

// Global test framework instance
TestFramework& get_test_framework();

// Main function for running tests
int run_tests(int argc, char* argv[]);

// Test registration macro
#define TEST(test_name) \
    void test_##test_name(); \
    struct TestRegistrar_##test_name { \
        TestRegistrar_##test_name() { \
            tang::test::get_test_framework().add_test(#test_name, test_##test_name); \
        } \
    } test_registrar_##test_name; \
    void test_##test_name()

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while(0)

#define ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expected << " == " << #actual << " at " << __FILE__ << ":" << __LINE__ \
               << " (expected: " << (expected) << ", actual: " << (actual) << ")"; \
            throw std::runtime_error(ss.str()); \
        } \
    } while(0)

#define ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expected << " != " << #actual << " at " << __FILE__ << ":" << __LINE__ \
               << " (both are: " << (expected) << ")"; \
            throw std::runtime_error(ss.str()); \
        } \
    } while(0)

#define ASSERT_THROWS(expression, exception_type) \
    do { \
        bool caught = false; \
        try { \
            expression; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expression << " should throw " << #exception_type << " at " << __FILE__ << ":" << __LINE__ \
               << " (threw different exception)"; \
            throw std::runtime_error(ss.str()); \
        } \
        if (!caught) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expression << " should throw " << #exception_type << " at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while(0)
} // namespace test
} // namespace tang