#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "../include/lexer.h"

int main(int argc, char* argv[]) {
    std::cout << "Tang Compiler v0.1" << std::endl;
    
    // Check command line arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    
    try {
        // Step 1: Read the source file
        std::ifstream file(input_file);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open source file: " + input_file);
        }
        
        std::string source_code((std::istreambuf_iterator<char>(file)), 
                               std::istreambuf_iterator<char>());
        file.close();
        
        std::cout << "Compiling " << input_file << " to " << output_file << "..." << std::endl;
        
        // Step 2: Lexical analysis
        std::cout << "  Step 1: Lexical analysis..." << std::endl;
        tang::Lexer lexer(source_code);
        
        // Test the lexer by getting all tokens
        int token_count = 0;
        tang::Token token;
        do {
            token = lexer.getNextToken();
            token_count++;
        } while (token.type != tang::END_OF_FILE);
        
        std::cout << "  Lexical analysis completed. Found " << token_count << " tokens." << std::endl;
        
        // Step 3: Syntax analysis (skipped for now)
        std::cout << "  Step 2: Syntax analysis... (skipped)" << std::endl;
        
        // Step 4: Semantic analysis (skipped for now)
        std::cout << "  Step 3: Semantic analysis... (skipped)" << std::endl;
        
        // Step 5: IR generation (not implemented yet)
        std::cout << "  Step 4: IR generation... (skipped)" << std::endl;
        
        // Step 6: Code generation (not implemented yet)
        std::cout << "  Step 5: Code generation... (skipped)" << std::endl;
        
        std::cout << "Compilation successful!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return 1;
    }
}
