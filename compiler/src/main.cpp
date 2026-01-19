#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/semantic_analyzer.h"
#include "../include/ir.h"

namespace tang {

class Compiler {
public:
    Compiler() {
        lexer = std::make_unique<Lexer>();
        parser = std::make_unique<Parser>();
        semantic_analyzer = std::make_unique<SemanticAnalyzer>();
    }
    
    bool compile(const std::string& source_code, const std::string& output_file) {
        try {
            std::cout << "=== Tang Compiler ===" << std::endl;
            
            // 1. Lexical analysis
            std::cout << "[1/5] Lexical analysis..." << std::endl;
            auto tokens = lexer->tokenize(source_code);
            std::cout << "    Generated " << tokens.size() << " tokens" << std::endl;
            
            // 2. Syntax analysis
            std::cout << "[2/5] Syntax analysis..." << std::endl;
            parser->setTokens(tokens);
            auto ast_module = parser->parseModule();
            std::cout << "    Parsed " << ast_module->functions.size() << " functions" << std::endl;
            
            // 3. Semantic analysis
            std::cout << "[3/5] Semantic analysis..." << std::endl;
            semantic_analyzer->analyzeModule(ast_module);
            std::cout << "    Semantic analysis completed successfully" << std::endl;
            
            // 4. IR generation
            std::cout << "[4/5] IR generation..." << std::endl;
            auto ir_module = ir::generateIR(ast_module);
            std::cout << "    Generated IR module" << std::endl;
            
            // 5. Code generation
            std::cout << "[5/5] Code generation..." << std::endl;
            auto code_generator = ir::createX86_64CodeGenerator();
            code_generator->generateCode(*ir_module, output_file);
            std::cout << "    Generated x86-64 assembly to: " << output_file << std::endl;
            
            std::cout << "=== Compilation successful! ===" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Compilation error: " << e.what() << std::endl;
            return false;
        }
    }
    
    void printTokens(const std::string& source_code) {
        auto tokens = lexer->tokenize(source_code);
        for (const auto& token : tokens) {
            std::cout << "[" << token.type << "] " << token.lexeme << std::endl;
        }
    }
    
    void printAST(const std::string& source_code) {
        auto tokens = lexer->tokenize(source_code);
        parser->setTokens(tokens);
        auto ast_module = parser->parseModule();
        
        // Simple AST printer
        for (const auto& func : ast_module->functions) {
            std::cout << "Function: " << func->name << " (" 
                      << (func->is_sync ? "sync" : "async") << ")" << std::endl;
            std::cout << "  Parameters: " << func->params.size() << std::endl;
            std::cout << "  Body statements: " << func->body.size() << std::endl;
        }
    }
    
    void printIR(const std::string& source_code) {
        auto tokens = lexer->tokenize(source_code);
        parser->setTokens(tokens);
        auto ast_module = parser->parseModule();
        semantic_analyzer->analyzeModule(ast_module);
        auto ir_module = ir::generateIR(ast_module);
        
        ir::printIR(*ir_module);
    }
    
private:
    std::unique_ptr<Lexer> lexer;
    std::unique_ptr<Parser> parser;
    std::unique_ptr<SemanticAnalyzer> semantic_analyzer;
};

} // namespace tang

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input.tang> <output.asm> [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --tokens    Print tokens" << std::endl;
        std::cout << "  --ast      Print AST" << std::endl;
        std::cout << "  --ir       Print IR" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    
    // Read source code
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << input_file << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_code = buffer.str();
    file.close();
    
    tang::Compiler compiler;
    
    // Handle options
    for (int i = 3; i < argc; ++i) {
        std::string option = argv[i];
        if (option == "--tokens") {
            compiler.printTokens(source_code);
            return 0;
        } else if (option == "--ast") {
            compiler.printAST(source_code);
            return 0;
        } else if (option == "--ir") {
            compiler.printIR(source_code);
            return 0;
        }
    }
    
    // Full compilation
    return compiler.compile(source_code, output_file) ? 0 : 1;
}