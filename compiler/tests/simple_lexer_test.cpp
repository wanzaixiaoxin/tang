#include <iostream>
#include <string>
#include "../include/lexer.h"

using namespace std;
using namespace tang;

int main() {
    // Simple test: Check if lexer can correctly recognize basic tokens
    string test_source = "let x: int = 42; let message: string = \"Hello, Tang!\"";
    Lexer lexer(test_source);
    
    cout << "Testing lexer with source: " << test_source << endl;
    cout << "====================================" << endl;
    
    Token token;
    int token_count = 0;
    
    do {
        token = lexer.getNextToken();
        token_count++;
        
        cout << "Token " << token_count << ": " << token.lexeme << " (Type: " << token.type << ")" << endl;
        
    } while (token.type != END_OF_FILE);
    
    cout << "====================================" << endl;
    cout << "Test completed. Found " << token_count << " tokens." << endl;
    
    return 0;
}