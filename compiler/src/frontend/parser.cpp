#include <iostream>
#include <stdexcept>
#include <memory>
#include "../../include/parser.h"
#include "../../include/ast.h"

namespace tang {

Parser::Parser(Lexer& lexer)
    : lexer(lexer)
{
    advance(); // 初始化为第一个标记
}

Token Parser::peek() const {
    return current_token;
}

Token Parser::advance() {
    current_token = lexer.getNextToken();
    return current_token;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    return current_token.type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        Token token = current_token;
        advance();
        return token;
    }
    error(current_token, message);
    throw std::runtime_error(message);
}

void Parser::error(const Token& token, const std::string& message) {
    std::cerr << "Error at line " << token.line << ", column " << token.column << ": " << message << std::endl;
    std::cerr << "Token: " << token.lexeme << " (" << token.type << ")" << std::endl;
}

std::shared_ptr<Module> Parser::parseModule() {
    auto module = std::make_shared<Module>();
    
    while (!check(END_OF_FILE)) {
        if (match(KEYWORD_FN) || (check(KEYWORD_SYNC) && peek(1).type == KEYWORD_FN)) {
            // 解析函数声明
            auto fn_decl = parseFunctionDecl();
            module->functions.push_back(fn_decl);
        } else if (match(KEYWORD_LET) || match(KEYWORD_CONST)) {
            // 解析全局变量声明
            auto var_decl = parseVarDeclStmt();
            module->global_vars.push_back(var_decl);
        } else {
            // 无效的顶级声明
            error(current_token, "Expected function or variable declaration");
            advance(); // 跳过无效标记
        }
    }
    
    return module;
}

std::shared_ptr<Type> Parser::parseType() {
    auto type = std::make_shared<Type>();
    
    if (match(TYPE_INT)) {
        type->kind = Type::INT;
        type->name = "int";
    } else if (match(TYPE_FLOAT)) {
        type->kind = Type::FLOAT;
        type->name = "float";
    } else if (match(TYPE_BOOL)) {
        type->kind = Type::BOOL;
        type->name = "bool";
    } else if (match(TYPE_STRING)) {
        type->kind = Type::STRING;
        type->name = "string";
    } else if (match(TYPE_VOID)) {
        type->kind = Type::VOID;
        type->name = "void";
    } else if (match(KEYWORD_RESULT)) {
        // 解析 Result<T, E> 类型
        type->kind = Type::RESULT;
        consume(SEP_LPAREN, "Expected '('");
        type->ok_type = parseType();
        consume(SEP_COMMA, "Expected ','");
        type->err_type = parseType();
        consume(SEP_RPAREN, "Expected ')'");
        type->name = "Result<" + type->ok_type->name + ", " + type->err_type->name + ">";
    } else if (check(IDENTIFIER)) {
        // 自定义类型
        type->kind = Type::CUSTOM;
        type->name = current_token.lexeme;
        advance();
    } else {
        error(current_token, "Expected type");
        advance();
        return type;
    }
    
    // 检查数组类型 []
    while (match(SEP_LBRACK)) {
        consume(SEP_RBRACK, "Expected ']'");
        auto array_type = std::make_shared<Type>();
        array_type->kind = Type::ARRAY;
        array_type->element_type = type;
        array_type->name = type->name + "[]";
        type = array_type;
    }
    
    return type;
}

std::shared_ptr<Expr> Parser::parseExpr() {
    return parseAssignmentExpr();
}

std::shared_ptr<Expr> Parser::parseAssignmentExpr() {
    auto expr = parseLogicalOrExpr();
    
    if (match(OP_ASSIGN)) {
        auto assign_expr = std::make_shared<AssignExpr>();
        assign_expr->target = std::dynamic_pointer_cast<IdentifierExpr>(expr);
        if (!assign_expr->target) {
            error(current_token, "Expected identifier as assignment target");
        }
        assign_expr->value = parseAssignmentExpr();
        return assign_expr;
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseLogicalOrExpr() {
    auto expr = parseLogicalAndExpr();
    
    while (match(OP_OR)) {
        auto binary_expr = std::make_shared<BinaryExpr>();
        binary_expr->op = BinaryExpr::OP_OR;
        binary_expr->left = expr;
        binary_expr->right = parseLogicalAndExpr();
        expr = binary_expr;
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseLogicalAndExpr() {
    auto expr = parseEqualityExpr();
    
    while (match(OP_AND)) {
        auto binary_expr = std::make_shared<BinaryExpr>();
        binary_expr->op = BinaryExpr::OP_AND;
        binary_expr->left = expr;
        binary_expr->right = parseEqualityExpr();
        expr = binary_expr;
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseEqualityExpr() {
    auto expr = parseComparisonExpr();
    
    while (true) {
        if (match(OP_EQ)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_EQ;
            binary_expr->left = expr;
            binary_expr->right = parseComparisonExpr();
            expr = binary_expr;
        } else if (match(OP_NEQ)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_NEQ;
            binary_expr->left = expr;
            binary_expr->right = parseComparisonExpr();
            expr = binary_expr;
        } else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseComparisonExpr() {
    auto expr = parseAdditiveExpr();
    
    while (true) {
        if (match(OP_LT)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_LT;
            binary_expr->left = expr;
            binary_expr->right = parseAdditiveExpr();
            expr = binary_expr;
        } else if (match(OP_LTE)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_LTE;
            binary_expr->left = expr;
            binary_expr->right = parseAdditiveExpr();
            expr = binary_expr;
        } else if (match(OP_GT)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_GT;
            binary_expr->left = expr;
            binary_expr->right = parseAdditiveExpr();
            expr = binary_expr;
        } else if (match(OP_GTE)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_GTE;
            binary_expr->left = expr;
            binary_expr->right = parseAdditiveExpr();
            expr = binary_expr;
        } else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseAdditiveExpr() {
    auto expr = parseMultiplicativeExpr();
    
    while (true) {
        if (match(OP_ADD)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_ADD;
            binary_expr->left = expr;
            binary_expr->right = parseMultiplicativeExpr();
            expr = binary_expr;
        } else if (match(OP_SUB)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_SUB;
            binary_expr->left = expr;
            binary_expr->right = parseMultiplicativeExpr();
            expr = binary_expr;
        } else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseMultiplicativeExpr() {
    auto expr = parseUnaryExpr();
    
    while (true) {
        if (match(OP_MUL)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_MUL;
            binary_expr->left = expr;
            binary_expr->right = parseUnaryExpr();
            expr = binary_expr;
        } else if (match(OP_DIV)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_DIV;
            binary_expr->left = expr;
            binary_expr->right = parseUnaryExpr();
            expr = binary_expr;
        } else if (match(OP_MOD)) {
            auto binary_expr = std::make_shared<BinaryExpr>();
            binary_expr->op = BinaryExpr::OP_MOD;
            binary_expr->left = expr;
            binary_expr->right = parseUnaryExpr();
            expr = binary_expr;
        } else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::parseUnaryExpr() {
    if (match(OP_NOT)) {
        auto unary_expr = std::make_shared<UnaryExpr>();
        unary_expr->op = UnaryExpr::OP_NOT;
        unary_expr->expr = parseUnaryExpr();
        return unary_expr;
    } else if (match(OP_SUB)) {
        auto unary_expr = std::make_shared<UnaryExpr>();
        unary_expr->op = UnaryExpr::OP_NEG;
        unary_expr->expr = parseUnaryExpr();
        return unary_expr;
    }
    
    return parsePrimaryExpr();
}

std::shared_ptr<Expr> Parser::parsePrimaryExpr() {
    if (match(IDENTIFIER)) {
        auto ident = std::make_shared<IdentifierExpr>();
        ident->name = current_token.lexeme;
        
        // 检查函数调用
        if (match(SEP_LPAREN)) {
            auto call_expr = std::make_shared<CallExpr>();
            call_expr->callee = ident;
            
            // 解析参数列表
            if (!check(SEP_RPAREN)) {
                do {
                    call_expr->args.push_back(parseExpr());
                } while (match(SEP_COMMA));
            }
            
            consume(SEP_RPAREN, "Expected ')' after function arguments");
            return call_expr;
        }
        
        // 检查数组访问
        if (match(SEP_LBRACK)) {
            auto array_access = std::make_shared<ArrayAccessExpr>();
            array_access->array = ident;
            array_access->index = parseExpr();
            consume(SEP_RBRACK, "Expected ']' after array index");
            return array_access;
        }
        
        return ident;
    }
    
    if (match(INT_LITERAL)) {
        auto literal = std::make_shared<LiteralExpr>();
        literal->kind = LiteralExpr::INT_LITERAL;
        literal->value = current_token.lexeme;
        return literal;
    }
    
    if (match(FLOAT_LITERAL)) {
        auto literal = std::make_shared<LiteralExpr>();
        literal->kind = LiteralExpr::FLOAT_LITERAL;
        literal->value = current_token.lexeme;
        return literal;
    }
    
    if (match(BOOL_LITERAL)) {
        auto literal = std::make_shared<LiteralExpr>();
        literal->kind = LiteralExpr::BOOL_LITERAL;
        literal->value = current_token.lexeme;
        return literal;
    }
    
    if (match(STRING_LITERAL)) {
        auto literal = std::make_shared<LiteralExpr>();
        literal->kind = LiteralExpr::STRING_LITERAL;
        literal->value = current_token.lexeme;
        return literal;
    }
    
    if (match(SEP_LPAREN)) {
        auto expr = parseExpr();
        consume(SEP_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error(current_token, "Expected expression");
    advance();
    return std::make_shared<IdentifierExpr>(); // 错误恢复
}

std::shared_ptr<Stmt> Parser::parseStmt() {
    if (check(KEYWORD_LET) || check(KEYWORD_CONST)) {
        return parseVarDeclStmt();
    } else if (check(KEYWORD_RETURN)) {
        return parseReturnStmt();
    } else if (check(KEYWORD_IF)) {
        return parseIfStmt();
    } else if (check(KEYWORD_WHILE)) {
        return parseWhileStmt();
    } else if (check(KEYWORD_FOR)) {
        return parseForStmt();
    } else if (check(SEP_LBRACE)) {
        // 语句块作为单个语句
        auto stmt_block = parseStmtBlock();
        // 将语句块转换为表达式语句（如果需要）
        // 这里简化处理，实际可能需要更复杂的结构
        return nullptr;
    } else {
        // 表达式语句
        auto expr = parseExpr();
        consume(SEP_SEMICOLON, "Expected ';' after expression");
        auto expr_stmt = std::make_shared<ExprStmt>();
        expr_stmt->expr = expr;
        return expr_stmt;
    }
}

std::shared_ptr<Stmt> Parser::parseVarDeclStmt() {
    bool is_const = false;
    if (check(KEYWORD_CONST)) {
        is_const = true;
        advance();
    } else {
        advance(); // KEYWORD_LET
    }
    
    auto var_decl = std::make_shared<VarDeclStmt>();
    var_decl->is_const = is_const;
    
    // 解析变量名
    consume(IDENTIFIER, "Expected variable name");
    var_decl->name = current_token.lexeme;
    
    // 解析类型注解
    consume(SEP_COLON, "Expected ':' after variable name");
    var_decl->type = parseType();
    
    // 解析初始化器
    if (match(OP_ASSIGN)) {
        var_decl->initializer = parseExpr();
    }
    
    consume(SEP_SEMICOLON, "Expected ';' after variable declaration");
    return var_decl;
}

std::shared_ptr<Stmt> Parser::parseReturnStmt() {
    advance(); // KEYWORD_RETURN
    
    auto return_stmt = std::make_shared<ReturnStmt>();
    
    if (!check(SEP_SEMICOLON)) {
        return_stmt->expr = parseExpr();
    }
    
    consume(SEP_SEMICOLON, "Expected ';' after return statement");
    return return_stmt;
}

std::shared_ptr<Stmt> Parser::parseIfStmt() {
    advance(); // KEYWORD_IF
    
    auto if_stmt = std::make_shared<IfStmt>();
    
    consume(SEP_LPAREN, "Expected '(' after if");
    if_stmt->condition = parseExpr();
    consume(SEP_RPAREN, "Expected ')' after if condition");
    
    // 解析 then 分支
    if (match(SEP_LBRACE)) {
        if_stmt->then_branch = parseStmtBlock();
    } else {
        auto stmt = parseStmt();
        if (stmt) {
            if_stmt->then_branch.push_back(stmt);
        }
    }
    
    // 解析 else 分支
    if (match(KEYWORD_ELSE)) {
        if (match(SEP_LBRACE)) {
            if_stmt->else_branch = parseStmtBlock();
        } else {
            auto stmt = parseStmt();
            if (stmt) {
                if_stmt->else_branch.push_back(stmt);
            }
        }
    }
    
    return if_stmt;
}

std::shared_ptr<Stmt> Parser::parseWhileStmt() {
    advance(); // KEYWORD_WHILE
    
    auto while_stmt = std::make_shared<WhileStmt>();
    
    consume(SEP_LPAREN, "Expected '(' after while");
    while_stmt->condition = parseExpr();
    consume(SEP_RPAREN, "Expected ')' after while condition");
    
    // 解析循环体
    if (match(SEP_LBRACE)) {
        while_stmt->body = parseStmtBlock();
    } else {
        auto stmt = parseStmt();
        if (stmt) {
            while_stmt->body.push_back(stmt);
        }
    }
    
    return while_stmt;
}

std::shared_ptr<Stmt> Parser::parseForStmt() {
    advance(); // KEYWORD_FOR
    
    consume(SEP_LPAREN, "Expected '(' after for");
    
    // 检查 for-in 循环
    if (check(KEYWORD_LET) || check(KEYWORD_CONST)) {
        // 解析初始化器（变量声明）
        auto init_stmt = parseVarDeclStmt();
        auto var_decl = std::dynamic_pointer_cast<VarDeclStmt>(init_stmt);
        
        if (match(KEYWORD_IN)) {
            // for-in 循环
            auto for_in_stmt = std::make_shared<ForInStmt>();
            for_in_stmt->var_name = var_decl->name;
            for_in_stmt->range = parseExpr();
            consume(SEP_RPAREN, "Expected ')' after for-in range");
            
            // 解析循环体
            if (match(SEP_LBRACE)) {
                for_in_stmt->body = parseStmtBlock();
            } else {
                auto stmt = parseStmt();
                if (stmt) {
                    for_in_stmt->body.push_back(stmt);
                }
            }
            
            return for_in_stmt;
        } else {
            // 传统 for 循环，回退并重新解析
            // 这里简化处理，实际需要更复杂的回退机制
            error(current_token, "Expected 'in' after for variable declaration");
            advance();
            return nullptr;
        }
    }
    
    // 传统 for 循环
    auto for_stmt = std::make_shared<ForStmt>();
    
    // 解析初始化表达式
    if (!check(SEP_SEMICOLON)) {
        auto expr = parseExpr();
        // 简化处理，实际可能需要解析变量声明
        consume(SEP_SEMICOLON, "Expected ';' after for initialization");
    } else {
        advance(); // SEP_SEMICOLON
    }
    
    // 解析条件表达式
    if (!check(SEP_SEMICOLON)) {
        for_stmt->condition = parseExpr();
    }
    consume(SEP_SEMICOLON, "Expected ';' after for condition");
    
    // 解析递增表达式
    if (!check(SEP_RPAREN)) {
        for_stmt->increment = parseExpr();
    }
    consume(SEP_RPAREN, "Expected ')' after for increment");
    
    // 解析循环体
    if (match(SEP_LBRACE)) {
        for_stmt->body = parseStmtBlock();
    } else {
        auto stmt = parseStmt();
        if (stmt) {
            for_stmt->body.push_back(stmt);
        }
    }
    
    return for_stmt;
}

std::vector<std::shared_ptr<Stmt>> Parser::parseStmtBlock() {
    std::vector<std::shared_ptr<Stmt>> stmts;
    
    consume(SEP_LBRACE, "Expected '{' at beginning of statement block");
    
    while (!check(SEP_RBRACE) && !check(END_OF_FILE)) {
        auto stmt = parseStmt();
        if (stmt) {
            stmts.push_back(stmt);
        }
    }
    
    consume(SEP_RBRACE, "Expected '}' at end of statement block");
    
    return stmts;
}

std::shared_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    bool is_sync = false;
    if (match(KEYWORD_SYNC)) {
        is_sync = true;
    }
    
    consume(KEYWORD_FN, "Expected 'fn' after 'sync' keyword");
    
    auto fn_decl = std::make_shared<FunctionDecl>();
    fn_decl->is_sync = is_sync;
    
    // 解析函数名
    consume(IDENTIFIER, "Expected function name");
    fn_decl->name = current_token.lexeme;
    
    // 解析参数列表
    consume(SEP_LPAREN, "Expected '(' after function name");
    
    if (!check(SEP_RPAREN)) {
        do {
            // 解析参数名
            consume(IDENTIFIER, "Expected parameter name");
            std::string param_name = current_token.lexeme;
            
            // 解析参数类型
            consume(SEP_COLON, "Expected ':' after parameter name");
            auto param_type = parseType();
            
            fn_decl->params.emplace_back(param_name, param_type);
        } while (match(SEP_COMMA));
    }
    
    consume(SEP_RPAREN, "Expected ')' after function parameters");
    
    // 解析返回类型
    if (match(SEP_ARROW)) {
        fn_decl->return_type = parseType();
    } else {
        // 默认返回类型为 void
        auto void_type = std::make_shared<Type>();
        void_type->kind = Type::VOID;
        void_type->name = "void";
        fn_decl->return_type = void_type;
    }
    
    // 解析函数体
    consume(SEP_LBRACE, "Expected '{' at beginning of function body");
    fn_decl->body = parseStmtBlock();
    
    return fn_decl;
}

} // namespace tang
