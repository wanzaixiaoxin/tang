#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include "ast.h"

namespace tang {

class SymbolTable {
public:
    SymbolTable() = default;
    SymbolTable(std::shared_ptr<SymbolTable> parent) : parent(parent) {}
    
    void addVariable(const std::string& name, std::shared_ptr<Type> type, bool is_const);
    bool hasVariable(const std::string& name) const;
    std::shared_ptr<Type> getVariableType(const std::string& name) const;
    bool isVariableConst(const std::string& name) const;
    
    void addFunction(const std::string& name, std::shared_ptr<FunctionDecl> func);
    bool hasFunction(const std::string& name) const;
    std::shared_ptr<FunctionDecl> getFunction(const std::string& name) const;
    
private:
    struct VariableInfo {
        std::shared_ptr<Type> type;
        bool is_const;
    };
    
    std::unordered_map<std::string, VariableInfo> variables;
    std::unordered_map<std::string, std::shared_ptr<FunctionDecl>> functions;
    std::shared_ptr<SymbolTable> parent;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    
    void analyze(const std::shared_ptr<Module>& module);
    
private:
    std::shared_ptr<SymbolTable> current_scope;
    
    // Type checking helper methods
    bool isTypeCompatible(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const;
    bool isTypeEqual(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const;
    
    // Analysis methods
    void analyzeModule(const std::shared_ptr<Module>& module);
    void analyzeFunction(const std::shared_ptr<FunctionDecl>& func);
    void analyzeStmt(const std::shared_ptr<Stmt>& stmt);
    void analyzeVarDecl(const std::shared_ptr<VarDeclStmt>& var_decl);
    void analyzeReturnStmt(const std::shared_ptr<ReturnStmt>& return_stmt, const std::shared_ptr<Type>& expected_type);
    void analyzeIfStmt(const std::shared_ptr<IfStmt>& if_stmt);
    void analyzeWhileStmt(const std::shared_ptr<WhileStmt>& while_stmt);
    void analyzeForStmt(const std::shared_ptr<ForStmt>& for_stmt);
    void analyzeForInStmt(const std::shared_ptr<ForInStmt>& for_in_stmt);
    
    // Expression analysis and type inference
    std::shared_ptr<Type> analyzeExpr(const std::shared_ptr<Expr>& expr);
    std::shared_ptr<Type> analyzeBinaryExpr(const std::shared_ptr<BinaryExpr>& binary_expr);
    std::shared_ptr<Type> analyzeUnaryExpr(const std::shared_ptr<UnaryExpr>& unary_expr);
    std::shared_ptr<Type> analyzeLiteralExpr(const std::shared_ptr<LiteralExpr>& literal_expr);
    std::shared_ptr<Type> analyzeIdentifierExpr(const std::shared_ptr<IdentifierExpr>& ident_expr);
    std::shared_ptr<Type> analyzeCallExpr(const std::shared_ptr<CallExpr>& call_expr);
    std::shared_ptr<Type> analyzeAssignExpr(const std::shared_ptr<AssignExpr>& assign_expr);
    std::shared_ptr<Type> analyzeArrayAccessExpr(const std::shared_ptr<ArrayAccessExpr>& array_access_expr);
    
    // Error handling
    void error(const Position& pos, const std::string& message);
    void warning(const Position& pos, const std::string& message);
};

} // namespace tang
