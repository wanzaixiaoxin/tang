#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/semantic_analyzer.h"
#include "../include/ast.h"

using namespace std;
using namespace tang;

// 简单的测试框架
void run_test(const string& test_name, function<bool()> test_func) {
    cout << "Running test: " << test_name << "... ";
    if (test_func()) {
        cout << "PASS" << endl;
    } else {
        cout << "FAIL" << endl;
    }
}

// 测试基本语义分析
bool test_basic_semantic() {
    string source = R"(
        fn main() -> int {
            let x: int = 10;
            const y: float = 3.14;
            let result: int = x + 5;
            
            if (x > 0) {
                return result;
            }
            
            return 0;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        SemanticAnalyzer analyzer;
        analyzer.analyze(module);
        
        return true;
    } catch (const exception& e) {
        cout << "Semantic analysis error: " << e.what() << endl;
        return false;
    }
}

// 测试类型错误检测
bool test_type_errors() {
    string source = R"(
        fn test_type_errors() {
            let x: int = 10;
            let y: string = "hello";
            
            // 应该检测到类型错误
            let z: int = x + y;  // int + string 应该报错
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        SemanticAnalyzer analyzer;
        analyzer.analyze(module);
        
        // 如果执行到这里，说明没有检测到类型错误，测试失败
        cout << "Expected type error was not detected" << endl;
        return false;
    } catch (const exception& e) {
        // 期望出现类型错误
        cout << "Correctly detected type error: " << e.what() << endl;
        return true;
    }
}

// 测试变量作用域
bool test_variable_scope() {
    string source = R"(
        fn test_scope() -> int {
            let x: int = 10;
            
            if (x > 0) {
                let y: int = 20;  // 内部作用域变量
                return x + y;
            }
            
            // 这里不能访问 y
            return x;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        SemanticAnalyzer analyzer;
        analyzer.analyze(module);
        
        return true;
    } catch (const exception& e) {
        cout << "Scope analysis error: " << e.what() << endl;
        return false;
    }
}

// 测试函数调用语义
bool test_function_calls() {
    string source = R"(
        fn add(a: int, b: int) -> int {
            return a + b;
        }
        
        fn test_calls() -> int {
            let x: int = 5;
            let y: int = 10;
            
            // 正确的函数调用
            let result: int = add(x, y);
            
            // 错误的函数调用（参数类型不匹配）
            let error_result: int = add(x, "string");
            
            return result;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        SemanticAnalyzer analyzer;
        analyzer.analyze(module);
        
        // 如果执行到这里，说明没有检测到参数类型错误
        cout << "Expected parameter type error was not detected" << endl;
        return false;
    } catch (const exception& e) {
        // 期望出现参数类型错误
        cout << "Correctly detected function call error: " << e.what() << endl;
        return true;
    }
}

// 测试常量语义
bool test_const_semantics() {
    string source = R"(
        fn test_const() {
            const x: int = 10;
            
            // 应该检测到常量重新赋值的错误
            x = 20;  // 错误：不能给常量赋值
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        SemanticAnalyzer analyzer;
        analyzer.analyze(module);
        
        // 如果执行到这里，说明没有检测到常量赋值错误
        cout << "Expected const assignment error was not detected" << endl;
        return false;
    } catch (const exception& e) {
        // 期望出现常量赋值错误
        cout << "Correctly detected const assignment error: " << e.what() << endl;
        return true;
    }
}

int main() {
    cout << "=== Tang Semantic Analyzer Tests ===" << endl;
    
    run_test("Basic semantic analysis", test_basic_semantic);
    run_test("Type error detection", test_type_errors);
    run_test("Variable scope analysis", test_variable_scope);
    run_test("Function call semantics", test_function_calls);
    run_test("Const semantics", test_const_semantics);
    
    cout << "=== Semantic tests completed ===" << endl;
    
    return 0;
}