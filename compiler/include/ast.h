#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tang {

// Position information
struct Position {
    int line;
    int column;
};

// Base AST node class
class ASTNode {
public:
    Position position;
    virtual ~ASTNode() = default;
};

// Type definition
class Type : public ASTNode {
public:
    enum Kind {
        INT,
        FLOAT,
        BOOL,
        STRING,
        VOID,
        ARRAY,
        FUNCTION,
        RESULT,
        CUSTOM
    };

    Kind kind;
    std::string name;
    std::shared_ptr<Type> element_type; // Array element type
    std::vector<std::shared_ptr<Type>> param_types; // Function parameter types
    std::shared_ptr<Type> return_type; // Function return type
    std::shared_ptr<Type> ok_type; // Result<T, E> T type
    std::shared_ptr<Type> err_type; // Result<T, E> E type
};

// Expression base class
class Expr : public ASTNode {
public:
    std::shared_ptr<Type> type;
};

// Identifier expression
class IdentifierExpr : public Expr {
public:
    std::string name;
};

// Literal expression
class LiteralExpr : public Expr {
public:
    enum Kind {
        INT_LITERAL,
        FLOAT_LITERAL,
        BOOL_LITERAL,
        STRING_LITERAL
    };

    Kind kind;
    std::string value;
};

// Binary expression
class BinaryExpr : public Expr {
public:
    enum Op {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        EQ,
        NEQ,
        LT,
        LTE,
        GT,
        GTE,
        AND,
        OR
    };

    Op op;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;
};

// Unary expression
class UnaryExpr : public Expr {
public:
    enum Op {
        NOT,
        NEG
    };

    Op op;
    std::shared_ptr<Expr> expr;
};

// Assignment expression
class AssignExpr : public Expr {
public:
    std::shared_ptr<IdentifierExpr> target;
    std::shared_ptr<Expr> value;
};

// Function call expression
class CallExpr : public Expr {
public:
    std::shared_ptr<Expr> callee;
    std::vector<std::shared_ptr<Expr>> args;
};

// Array access expression
class ArrayAccessExpr : public Expr {
public:
    std::shared_ptr<Expr> array;
    std::shared_ptr<Expr> index;
};

// Statement base class
class Stmt : public ASTNode {
};

// Expression statement
class ExprStmt : public Stmt {
public:
    std::shared_ptr<Expr> expr;
};

// Variable declaration statement
class VarDeclStmt : public Stmt {
public:
    bool is_const;
    std::string name;
    std::shared_ptr<Type> type;
    std::shared_ptr<Expr> initializer;
};

// Return statement
class ReturnStmt : public Stmt {
public:
    std::shared_ptr<Expr> expr;
};

// Yield statement (for async functions)
class YieldStmt : public Stmt {
public:
    std::shared_ptr<Expr> expr;
};

// If statement
class IfStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::vector<std::shared_ptr<Stmt>> then_branch;
    std::vector<std::shared_ptr<Stmt>> else_branch;
};

// While statement
class WhileStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::vector<std::shared_ptr<Stmt>> body;
};

// For loop statement
class ForStmt : public Stmt {
public:
    std::shared_ptr<VarDeclStmt> init;
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Expr> increment;
    std::vector<std::shared_ptr<Stmt>> body;
};

// For-in loop statement
class ForInStmt : public Stmt {
public:
    std::string var_name;
    std::shared_ptr<Expr> range;
    std::vector<std::shared_ptr<Stmt>> body;
};

// Match statement (pattern matching)
class MatchStmt : public Stmt {
public:
    std::shared_ptr<Expr> expr;
    std::vector<std::pair<std::shared_ptr<Expr>, std::vector<std::shared_ptr<Stmt>>>> cases;
};

// Generic type parameter
class TypeParam : public ASTNode {
public:
    std::string name;
    std::shared_ptr<Type> constraint;
};

// Function declaration
class FunctionDecl : public ASTNode {
public:
    bool is_sync;
    std::string name;
    std::shared_ptr<Type> return_type;
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> params;
    std::vector<std::shared_ptr<Stmt>> body;
};

// Generic function declaration
class GenericFunctionDecl : public FunctionDecl {
public:
    std::vector<std::shared_ptr<TypeParam>> type_params;
};

// Module (entire file)
class Module : public ASTNode {
public:
    std::vector<std::shared_ptr<FunctionDecl>> functions;
    std::vector<std::shared_ptr<VarDeclStmt>> global_vars;
};

} // namespace tang
