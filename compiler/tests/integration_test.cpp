#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/semantic_analyzer.h"
#include "../include/ir.h"

namespace tang {

class IntegrationTest {
public:
    void runAllTests() {
        std::cout << "=== Running Tang Compiler Integration Tests ===" << std::endl;
        
        testLexer();
        testParser();
        testSemanticAnalysis();
        testIRGeneration();
        
        std::cout << "=== All tests passed! ===" << std::endl;
    }
    
private:
    void testLexer() {
        std::cout << "[1/4] Testing Lexer..." << std::endl;
        
        Lexer lexer;
        
        // Test basic tokenization
        std::string code = "func main() -> int { return 42; }";
        auto tokens = lexer.tokenize(code);
        
        assert(tokens.size() > 0);
        assert(tokens[0].type == TOKEN_KEYWORD_FUNC);
        assert(tokens[1].type == TOKEN_IDENTIFIER);
        assert(tokens[1].value == "main");
        
        std::cout << "    ✓ Basic tokenization passed" << std::endl;
        
        // Test async function tokens
        code = "async func test() -> int { yield 1; }";
        tokens = lexer.tokenize(code);
        
        assert(tokens.size() > 0);
        assert(tokens[0].type == TOKEN_KEYWORD_ASYNC);
        assert(tokens[2].type == TOKEN_IDENTIFIER);
        
        std::cout << "    ✓ Async function tokens passed" << std::endl;
        
        std::cout << "    Lexer tests completed successfully" << std::endl;
    }
    
    void testParser() {
        std::cout << "[2/4] Testing Parser..." << std::endl;
        
        Lexer lexer;
        Parser parser;
        
        // Test function parsing
        std::string code = "func add(a: int, b: int) -> int { return a + b; }";
        auto tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        auto module = parser.parseModule();
        assert(module != nullptr);
        assert(module->functions.size() == 1);
        
        auto func = module->functions[0];
        assert(func->name == "add");
        assert(func->params.size() == 2);
        assert(func->body.size() == 1);
        
        std::cout << "    ✓ Function parsing passed" << std::endl;
        
        // Test async function parsing
        code = "async func async_add(a: int, b: int) -> int { yield a + b; return 0; }";
        tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        module = parser.parseModule();
        assert(module != nullptr);
        assert(module->functions.size() == 1);
        
        func = module->functions[0];
        assert(func->name == "async_add");
        assert(!func->is_sync); // Should be async
        
        std::cout << "    ✓ Async function parsing passed" << std::endl;
        
        // Test control flow parsing
        code = "func test() -> int { if true { return 1; } else { return 2; } }";
        tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        module = parser.parseModule();
        assert(module != nullptr);
        
        std::cout << "    ✓ Control flow parsing passed" << std::endl;
        
        std::cout << "    Parser tests completed successfully" << std::endl;
    }
    
    void testSemanticAnalysis() {
        std::cout << "[3/4] Testing Semantic Analysis..." << std::endl;
        
        Lexer lexer;
        Parser parser;
        SemanticAnalyzer analyzer;
        
        // Test valid program
        std::string code = "func main() -> int { let x = 10; let y = 20; return x + y; }";
        auto tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        auto module = parser.parseModule();
        
        // Should not throw
        analyzer.analyzeModule(module);
        
        std::cout << "    ✓ Valid program analysis passed" << std::endl;
        
        // Test type checking
        code = "func test() -> int { let x: int = 10; let y: string = \"hello\"; return x + y; }"; // Should fail
        tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        module = parser.parseModule();
        
        bool caught_error = false;
        try {
            analyzer.analyzeModule(module);
        } catch (const std::exception&) {
            caught_error = true;
        }
        
        assert(caught_error); // Should catch type error
        
        std::cout << "    ✓ Type checking passed" << std::endl;
        
        std::cout << "    Semantic analysis tests completed successfully" << std::endl;
    }
    
    void testIRGeneration() {
        std::cout << "[4/4] Testing IR Generation..." << std::endl;
        
        Lexer lexer;
        Parser parser;
        SemanticAnalyzer analyzer;
        
        // Test basic IR generation
        std::string code = "func main() -> int { return 42; }";
        auto tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        auto module = parser.parseModule();
        analyzer.analyzeModule(module);
        
        auto ir_module = ir::generateIR(module);
        
        assert(ir_module.functions.size() == 1);
        assert(ir_module.functions[0].basic_blocks.size() > 0);
        
        std::cout << "    ✓ Basic IR generation passed" << std::endl;
        
        // Test async function IR generation
        code = "async func async_test() -> int { yield 1; return 2; }";
        tokens = lexer.tokenize(code);
        parser.setTokens(tokens);
        
        module = parser.parseModule();
        analyzer.analyzeModule(module);
        
        ir_module = ir::generateIR(module);
        
        assert(ir_module.functions.size() == 1);
        
        // Check for coroutine operations in IR
        bool has_coro_ops = false;
        for (const auto& func : ir_module.functions) {
            for (const auto& bb : func.basic_blocks) {
                for (const auto& instr : bb.instructions) {
                    if (instr.op_code == ir::OP_CORO_YIELD || 
                        instr.op_code == ir::OP_CORO_CREATE) {
                        has_coro_ops = true;
                        break;
                    }
                }
            }
        }
        
        assert(has_coro_ops); // Should have coroutine operations
        
        std::cout << "    ✓ Async function IR generation passed" << std::endl;
        
        // Test code generation
        std::string output_file = "test_output.asm";
        auto code_generator = ir::createX86_64CodeGenerator();
        
        // Should not throw
        code_generator->generateCode(ir_module, output_file);
        
        // Verify file was created
        std::ifstream file(output_file);
        assert(file.good());
        file.close();
        
        // Clean up
        std::remove(output_file.c_str());
        
        std::cout << "    ✓ Code generation passed" << std::endl;
        
        std::cout << "    IR generation tests completed successfully" << std::endl;
    }
};

} // namespace tang

int main() {
    tang::IntegrationTest test;
    test.runAllTests();
    return 0;
}