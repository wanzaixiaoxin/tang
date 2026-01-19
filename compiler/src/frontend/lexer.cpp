#include <cctype>
#include <string>
#include <unordered_map>
#include "../../include/lexer.h"

namespace tang {

Lexer::Lexer(const std::string& source)
    : source(source)
{}

char Lexer::peek() const {
    if (current >= source.size()) {
        return '\0';
    }
    return source[current];
}

char Lexer::advance() {
    char c = source[current];
    current++;
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (current >= source.size()) {
        return false;
    }
    if (source[current] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (isspace(c)) {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    // Skip single-line comments
    if (peek() == '/' && source[current + 1] == '/') {
        while (peek() != '\n' && peek() != '\0') {
            advance();
        }
        if (peek() == '\n') {
            advance();
        }
    }
}

Token Lexer::identifier() {
    int start = current;
    
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }
    
    std::string text = source.substr(start, current - start);
    TokenType type = checkKeyword(text);
    
    Token token;
    token.type = type;
    token.lexeme = text;
    token.line = line;
    token.column = column - (current - start);
    
    return token;
}

Token Lexer::number() {
    int start = current;
    bool is_float = false;
    
    while (isdigit(peek())) {
        advance();
    }
    
    if (peek() == '.' && isdigit(source[current + 1])) {
        is_float = true;
        advance();
        while (isdigit(peek())) {
            advance();
        }
    }
    
    Token token;
    token.type = is_float ? FLOAT_LITERAL : INT_LITERAL;
    token.lexeme = source.substr(start, current - start);
    token.line = line;
    token.column = column - (current - start);
    
    return token;
}

Token Lexer::stringLiteral() {
    int start = current + 1; // Skip opening quote
    advance();
    
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\n') {
            line++;
        }
        advance();
    }
    
    Token token;
    if (peek() == '\0') {
        // Unclosed string
        token.type = INVALID_TOKEN;
        token.lexeme = source.substr(start - 1, current - (start - 1));
    } else {
        advance(); // Skip closing quote
        token.type = STRING_LITERAL;
        token.lexeme = source.substr(start, current - start - 1);
    }
    
    token.line = line;
    token.column = column - (current - (start - 1));
    
    return token;
}

TokenType Lexer::checkKeyword(const std::string& identifier) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let", KEYWORD_LET},
        {"const", KEYWORD_CONST},
        {"fn", KEYWORD_FN},
        {"sync", KEYWORD_SYNC},
        {"if", KEYWORD_IF},
        {"else", KEYWORD_ELSE},
        {"while", KEYWORD_WHILE},
        {"for", KEYWORD_FOR},
        {"in", KEYWORD_IN},
        {"return", KEYWORD_RETURN},
        {"true", BOOL_LITERAL},
        {"false", BOOL_LITERAL},
        {"Ok", KEYWORD_OK},
        {"Err", KEYWORD_ERR},
        {"Result", KEYWORD_RESULT},
        {"match", KEYWORD_MATCH},
        {"int", TYPE_INT},
        {"float", TYPE_FLOAT},
        {"bool", TYPE_BOOL},
        {"string", TYPE_STRING},
        {"void", TYPE_VOID}
    };
    
    auto it = keywords.find(identifier);
    if (it != keywords.end()) {
        return it->second;
    }
    
    return IDENTIFIER;
}

Token Lexer::getNextToken() {
    while (true) {
        skipWhitespace();
        skipComment();
        
        char c = peek();
        
        if (c == '\0') {
            Token token;
            token.type = END_OF_FILE;
            token.lexeme = "";
            token.line = line;
            token.column = column;
            return token;
        }
        
        int start_column = column;
        
        if (isalpha(c) || c == '_') {
            return identifier();
        }
        
        if (isdigit(c)) {
            return number();
        }
        
        switch (c) {
            case '"':
                return stringLiteral();
            case '+':
                advance();
                return Token{OP_ADD, "+", line, start_column};
            case '-':
                advance();
                if (match('>')) {
                    return Token{SEP_ARROW, "->", line, start_column};
                }
                return Token{OP_SUB, "-", line, start_column};
            case '*':
                advance();
                return Token{OP_MUL, "*", line, start_column};
            case '/':
                advance();
                if (match('/')) {
                    skipComment();
                    continue;
                }
                return Token{OP_DIV, "/", line, start_column};
            case '%':
                advance();
                return Token{OP_MOD, "%", line, start_column};
            case '=':
                advance();
                if (match('=')) {
                    return Token{OP_EQ, "==", line, start_column};
                }
                return Token{OP_ASSIGN, "=", line, start_column};
            case '!':
                advance();
                if (match('=')) {
                    return Token{OP_NEQ, "!=", line, start_column};
                }
                return Token{OP_NOT, "!", line, start_column};
            case '<':
                advance();
                if (match('=')) {
                    return Token{OP_LTE, "<=", line, start_column};
                }
                return Token{SEP_LT, "<", line, start_column};
            case '>':
                advance();
                if (match('=')) {
                    return Token{OP_GTE, ">=", line, start_column};
                }
                return Token{SEP_GT, ">", line, start_column};
            case '&':
                advance();
                if (match('&')) {
                    return Token{OP_AND, "&&", line, start_column};
                }
                return Token{INVALID_TOKEN, "&", line, start_column};
            case '|':
                advance();
                if (match('|')) {
                    return Token{OP_OR, "||", line, start_column};
                }
                return Token{INVALID_TOKEN, "|", line, start_column};
            case '(':
                advance();
                return Token{SEP_LPAREN, "(", line, start_column};
            case ')':
                advance();
                return Token{SEP_RPAREN, ")", line, start_column};
            case '{':
                advance();
                return Token{SEP_LBRACE, "{", line, start_column};
            case '}':
                advance();
                return Token{SEP_RBRACE, "}", line, start_column};
            case '[':
                advance();
                return Token{SEP_LBRACK, "[", line, start_column};
            case ']':
                advance();
                return Token{SEP_RBRACK, "]", line, start_column};
            case ',':
                advance();
                return Token{SEP_COMMA, ",", line, start_column};
            case ':':
                advance();
                return Token{SEP_COLON, ":", line, start_column};
            case ';':
                advance();
                return Token{SEP_SEMICOLON, ";", line, start_column};
            case '.':
                advance();
                if (match('.')) {
                    return Token{SEP_DOT_DOT, "..", line, start_column};
                }
                return Token{SEP_DOT, ".", line, start_column};
            default:
                advance();
                return Token{INVALID_TOKEN, std::string(1, c), line, start_column};
        }
    }
}

std::vector<Token> Lexer::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    Lexer lexer(source);
    
    Token token;
    do {
        token = lexer.getNextToken();
        tokens.push_back(token);
    } while (token.type != END_OF_FILE);
    
    return tokens;
}

} // namespace tang