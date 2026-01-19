#pragma once

#include <memory>
#include "lexer.h"
#include "ast.h"

namespace tang {

class Parser {
public:
    explicit Parser(Lexer& lexer);
    
    // 解析整个模块
    std::shared_ptr<Module> parseModule();
    
private:
    // 辅助方法
    Token peek() const;
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& message);
    
    // 解析类型
    std::shared_ptr<Type> parseType();
    
    // 解析表达式
    std::shared_ptr<Expr> parseExpr();
    std::shared_ptr<Expr> parseAssignmentExpr();
    std::shared_ptr<Expr> parseLogicalOrExpr();
    std::shared_ptr<Expr> parseLogicalAndExpr();
    std::shared_ptr<Expr> parseEqualityExpr();
    std::shared_ptr<Expr> parseComparisonExpr();
    std::shared_ptr<Expr> parseAdditiveExpr();
    std::shared_ptr<Expr> parseMultiplicativeExpr();
    std::shared_ptr<Expr> parseUnaryExpr();
    std::shared_ptr<Expr> parsePrimaryExpr();
    
    // 解析语句
    std::shared_ptr<Stmt> parseStmt();
    std::shared_ptr<Stmt> parseVarDeclStmt();
    std::shared_ptr<Stmt> parseReturnStmt();
    std::shared_ptr<Stmt> parseIfStmt();
    std::shared_ptr<Stmt> parseWhileStmt();
    std::shared_ptr<Stmt> parseForStmt();
    std::vector<std::shared_ptr<Stmt>> parseStmtBlock();
    
    // 解析函数
    std::shared_ptr<FunctionDecl> parseFunctionDecl();
    
    // 错误处理
    void error(const Token& token, const std::string& message);
    
    // 状态
    Lexer& lexer;
    Token current_token;
};

} // namespace tang
