#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include "../include/lexer.h"
#include "../include/parser.h"
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

// 测试基本语法分析
bool test_basic_parsing() {
    string source = R"(
        fn main() {
            let x: int = 10;
            const y: float = 3.14;
            
            if (x > 0) {
                println("positive");
            }
            
            return x + y;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        // 检查模块结构
        if (module->functions.size() != 1) {
            cout << "Expected 1 function, got " << module->functions.size() << endl;
            return false;
        }
        
        auto main_func = module->functions[0];
        if (main_func->name != "main") {
            cout << "Expected function name 'main', got '" << main_func->name << "'" << endl;
            return false;
        }
        
        return true;
    } catch (const exception& e) {
        cout << "Parsing error: " << e.what() << endl;
        return false;
    }
}

// 测试异步函数解析
bool test_async_function() {
    string source = R"(
        async fn fetch_data(url: string) -> Result<string, Error> {
            return Ok("data");
        }
        
        sync fn process_sync() -> int {
            return 42;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        if (module->functions.size() != 2) {
            cout << "Expected 2 functions, got " << module->functions.size() << endl;
            return false;
        }
        
        // 检查异步函数
        auto async_func = module->functions[0];
        if (async_func->name != "fetch_data" || async_func->is_sync) {
            cout << "Async function parsing failed" << endl;
            return false;
        }
        
        // 检查同步函数
        auto sync_func = module->functions[1];
        if (sync_func->name != "process_sync" || !sync_func->is_sync) {
            cout << "Sync function parsing failed" << endl;
            return false;
        }
        
        return true;
    } catch (const exception& e) {
        cout << "Parsing error: " << e.what() << endl;
        return false;
    }
}

// 测试控制流语句
bool test_control_flow() {
    string source = R"(
        fn test_control() {
            let x: int = 10;
            
            // if-else
            if (x > 0) {
                println("positive");
            } else if (x < 0) {
                println("negative");
            } else {
                println("zero");
            }
            
            // while loop
            while (x > 0) {
                x = x - 1;
            }
            
            // for loop
            for (let i: int = 0; i < 10; i = i + 1) {
                println("i = ", i);
            }
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        if (module->functions.size() != 1) {
            cout << "Expected 1 function, got " << module->functions.size() << endl;
            return false;
        }
        
        auto func = module->functions[0];
        if (func->body.size() < 3) { // 至少应该有变量声明和几个控制流语句
            cout << "Expected more statements in function body" << endl;
            return false;
        }
        
        return true;
    } catch (const exception& e) {
        cout << "Parsing error: " << e.what() << endl;
        return false;
    }
}

// 测试表达式解析
bool test_expressions() {
    string source = R"(
        fn test_expr() -> int {
            let a: int = 10;
            let b: int = 20;
            
            // 算术运算
            let sum: int = a + b;
            let product: int = a * b;
            
            // 逻辑运算
            let cond1: bool = a > 0 && b < 30;
            let cond2: bool = a == 10 || b == 15;
            
            // 函数调用
            let result: int = add(a, b);
            
            // 数组访问
            let arr: int[] = [1, 2, 3];
            let elem: int = arr[0];
            
            return result;
        }
    )";
    
    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto module = parser.parseModule();
        
        return module->functions.size() == 1;
    } catch (const exception& e) {
        cout << "Expression parsing error: " << e.what() << endl;
        return false;
    }
}

int main() {
    cout << "=== Tang Parser Tests ===" << endl;
    
    run_test("Basic parsing", test_basic_parsing);
    run_test("Async function parsing", test_async_function);
    run_test("Control flow parsing", test_control_flow);
    run_test("Expression parsing", test_expressions);
    
    cout << "=== Tests completed ===" << endl;
    
    return 0;
}