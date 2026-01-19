#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include "../include/lexer.h"

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

// 测试词法分析器对关键字的识别
bool test_keywords() {
    string source = "let const fn sync if else while for in return true false Ok Err Result int float bool string void";
    Lexer lexer(source);
    
    vector<TokenType> expected = {
        KEYWORD_LET, KEYWORD_CONST, KEYWORD_FN, KEYWORD_SYNC, KEYWORD_IF, 
        KEYWORD_ELSE, KEYWORD_WHILE, KEYWORD_FOR, KEYWORD_IN, KEYWORD_RETURN, 
        BOOL_LITERAL, BOOL_LITERAL, KEYWORD_OK, KEYWORD_ERR, KEYWORD_RESULT, 
        TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STRING, TYPE_VOID, END_OF_FILE
    };
    
    for (const auto& expected_type : expected) {
        Token token = lexer.getNextToken();
        if (token.type != expected_type) {
            cout << "Expected token type " << expected_type << ", got " << token.type << endl;
            return false;
        }
    }
    
    return true;
}

// 测试词法分析器对标识符的识别
bool test_identifiers() {
    string test_source = "x _y z123 abc_def";
    Lexer test_lexer(test_source);
    
    for (int i = 0; i < 4; i++) {
        Token token = test_lexer.getNextToken();
        if (token.type != IDENTIFIER) {
            cout << "Expected IDENTIFIER, got " << token.type << endl;
            return false;
        }
    }
    
    Token token = test_lexer.getNextToken();
    if (token.type != END_OF_FILE) {
        cout << "Expected END_OF_FILE, got " << token.type << endl;
        return false;
    }
    
    return true;
}

// 测试词法分析器对数字的识别
bool test_numbers() {
    string test_source = "123 45.67 0.123 123.";
    Lexer test_lexer(test_source);
    
    Token token1 = test_lexer.getNextToken();
    if (token1.type != INT_LITERAL) {
        cout << "Expected INT_LITERAL, got " << token1.type << endl;
        return false;
    }
    if (token1.lexeme != "123") {
        cout << "Expected '123', got '" << token1.lexeme << "'" << endl;
        return false;
    }
    
    Token token2 = test_lexer.getNextToken();
    if (token2.type != FLOAT_LITERAL) {
        cout << "Expected FLOAT_LITERAL, got " << token2.type << endl;
        return false;
    }
    if (token2.lexeme != "45.67") {
        cout << "Expected '45.67', got '" << token2.lexeme << "'" << endl;
        return false;
    }
    
    Token token3 = test_lexer.getNextToken();
    if (token3.type != FLOAT_LITERAL) {
        cout << "Expected FLOAT_LITERAL, got " << token3.type << endl;
        return false;
    }
    if (token3.lexeme != "0.123") {
        cout << "Expected '0.123', got '" << token3.lexeme << "'" << endl;
        return false;
    }
    
    Token token4 = test_lexer.getNextToken();
    if (token4.type != INVALID_TOKEN) {
        cout << "Expected INVALID_TOKEN for '123.', got " << token4.type << endl;
        return false;
    }
    
    return true;
}

// 测试词法分析器对字符串的识别
bool test_strings() {
    string test_source = "\"hello\" \"world 123\" \"\"";
    Lexer test_lexer(test_source);
    
    Token token1 = test_lexer.getNextToken();
    if (token1.type != STRING_LITERAL) {
        cout << "Expected STRING_LITERAL, got " << token1.type << endl;
        return false;
    }
    if (token1.lexeme != "hello") {
        cout << "Expected 'hello', got '" << token1.lexeme << "'" << endl;
        return false;
    }
    
    Token token2 = test_lexer.getNextToken();
    if (token2.type != STRING_LITERAL) {
        cout << "Expected STRING_LITERAL, got " << token2.type << endl;
        return false;
    }
    if (token2.lexeme != "world 123") {
        cout << "Expected 'world 123', got '" << token2.lexeme << "'" << endl;
        return false;
    }
    
    Token token3 = test_lexer.getNextToken();
    if (token3.type != STRING_LITERAL) {
        cout << "Expected STRING_LITERAL, got " << token3.type << endl;
        return false;
    }
    if (token3.lexeme != "") {
        cout << "Expected empty string, got '" << token3.lexeme << "'" << endl;
        return false;
    }
    
    return true;
}

// 测试词法分析器对运算符的识别
bool test_operators() {
    string test_source = "+ - * / % = == ! != < <= > >= && ||";
    Lexer test_lexer(test_source);
    
    vector<TokenType> expected = {
        OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_ASSIGN, OP_EQ, OP_NOT, 
        OP_NEQ, OP_LT, OP_LTE, OP_GT, OP_GTE, OP_AND, OP_OR, END_OF_FILE
    };
    
    for (const auto& expected_type : expected) {
        Token token = test_lexer.getNextToken();
        if (token.type != expected_type) {
            cout << "Expected token type " << expected_type << ", got " << token.type << endl;
            return false;
        }
    }
    
    return true;
}

// 测试词法分析器对分隔符的识别
bool test_separators() {
    string test_source = "( ) { } [ ] , : ; -> . ..";
    Lexer test_lexer(test_source);
    
    vector<TokenType> expected = {
        SEP_LPAREN, SEP_RPAREN, SEP_LBRACE, SEP_RBRACE, SEP_LBRACK, SEP_RBRACK,
        SEP_COMMA, SEP_COLON, SEP_SEMICOLON, SEP_ARROW, SEP_DOT, SEP_DOT_DOT, END_OF_FILE
    };
    
    for (const auto& expected_type : expected) {
        Token token = test_lexer.getNextToken();
        if (token.type != expected_type) {
            cout << "Expected token type " << expected_type << ", got " << token.type << endl;
            return false;
        }
    }
    
    return true;
}

// 测试词法分析器对混合内容的识别
bool test_mixed_content() {
    string test_source = "fn main() { let x: int = 42; let y: float = 3.14; let message: string = \"Hello, Tang!\"; if x > y { return Ok(x); } else { return Err(\"Error message\"); } }";
    
    Lexer test_lexer(test_source);
    
    // 只需检查词法分析器能正确处理混合内容，不崩溃即可
    Token token;
    int token_count = 0;
    do {
        token = test_lexer.getNextToken();
        token_count++;
        // 确保所有令牌都有有效的类型
        if (token.type == INVALID_TOKEN && !token.lexeme.empty()) {
            cout << "Unexpected invalid token: '" << token.lexeme << "'" << endl;
            return false;
        }
    } while (token.type != END_OF_FILE);
    
    // 确保识别到了足够的令牌
    return token_count > 10; // 应该识别到至少10个令牌
}

int main() {
    cout << "Running lexer tests..." << endl;
    cout << "====================================" << endl;
    
    run_test("Keywords", test_keywords);
    run_test("Identifiers", test_identifiers);
    run_test("Numbers", test_numbers);
    run_test("Strings", test_strings);
    run_test("Operators", test_operators);
    run_test("Separators", test_separators);
    run_test("Mixed Content", test_mixed_content);
    
    cout << "====================================" << endl;
    cout << "All tests completed." << endl;
    
    return 0;
}