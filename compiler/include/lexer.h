#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace tang {

// Token types
enum TokenType {
    // Keywords
    KEYWORD_LET,
    KEYWORD_CONST,
    KEYWORD_FN,
    KEYWORD_SYNC,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_IN,
    KEYWORD_RETURN,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_OK,
    KEYWORD_ERR,
    KEYWORD_RESULT,
    KEYWORD_MATCH,
    
    // Types
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,
    
    // Operators
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_AND,
    OP_OR,
    OP_NOT,
    
    // Assignment
    OP_ASSIGN,
    
    // Separators
    SEP_LPAREN,
    SEP_RPAREN,
    SEP_LBRACE,
    SEP_RBRACE,
    SEP_LBRACK,
    SEP_RBRACK,
    SEP_COMMA,
    SEP_COLON,
    SEP_SEMICOLON,
    SEP_ARROW,
    SEP_DOT,
    SEP_DOT_DOT,
    SEP_LT,
    SEP_GT,
    
    // Identifiers and literals
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    BOOL_LITERAL,
    STRING_LITERAL,
    
    // Other
    END_OF_FILE,
    INVALID_TOKEN
};

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

// Lexer class
class Lexer {
public:
    explicit Lexer(const std::string& source);
    Lexer() = default; // Default constructor for testing
    
    // Get next token
    Token getNextToken();
    
    // Tokenize entire source code
    std::vector<Token> tokenize(const std::string& source);
    
private:
    // Helper methods
    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    void skipComment();
    
    // Token generation methods
    Token identifier();
    Token number();
    Token stringLiteral();
    
    // Keyword check
    TokenType checkKeyword(const std::string& identifier);
    
    // State
    std::string source;
    size_t current = 0;
    int line = 1;
    int column = 1;
};

} // namespace tang
