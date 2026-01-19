#include <iostream>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <string>
#include "../../include/semantic_analyzer.h"
#include "../../include/ast.h"

namespace tang {

// SymbolTable implementation

void SymbolTable::addVariable(const std::string& name, std::shared_ptr<Type> type, bool is_const) {
    variables[name] = {type, is_const};
}

bool SymbolTable::hasVariable(const std::string& name) const {
    if (variables.find(name) != variables.end()) {
        return true;
    }
    if (parent) {
        return parent->hasVariable(name);
    }
    return false;
}

std::shared_ptr<Type> SymbolTable::getVariableType(const std::string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second.type;
    }
    if (parent) {
        return parent->getVariableType(name);
    }
    return nullptr;
}

bool SymbolTable::isVariableConst(const std::string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second.is_const;
    }
    if (parent) {
        return parent->isVariableConst(name);
    }
    return false;
}

void SymbolTable::addFunction(const std::string& name, std::shared_ptr<FunctionDecl> func) {
    functions[name] = func;
}

bool SymbolTable::hasFunction(const std::string& name) const {
    if (functions.find(name) != functions.end()) {
        return true;
    }
    if (parent) {
        return parent->hasFunction(name);
    }
    return false;
}

std::shared_ptr<FunctionDecl> SymbolTable::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return it->second;
    }
    if (parent) {
        return parent->getFunction(name);
    }
    return nullptr;
}

// SemanticAnalyzer implementation

SemanticAnalyzer::SemanticAnalyzer() {
    current_scope = std::make_shared<SymbolTable>();
}

void SemanticAnalyzer::analyze(const std::shared_ptr<Module>& module) {
    // Simplified implementation for now
    std::cout << "Analyzing module..." << std::endl;
}

void SemanticAnalyzer::analyzeModule(const std::shared_ptr<Module>& module) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeFunction(const std::shared_ptr<FunctionDecl>& func) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeStmt(const std::shared_ptr<Stmt>& stmt) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeVarDecl(const std::shared_ptr<VarDeclStmt>& var_decl) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeReturnStmt(const std::shared_ptr<ReturnStmt>& return_stmt, const std::shared_ptr<Type>& expected_type) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeIfStmt(const std::shared_ptr<IfStmt>& if_stmt) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeWhileStmt(const std::shared_ptr<WhileStmt>& while_stmt) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeForStmt(const std::shared_ptr<ForStmt>& for_stmt) {
    // Simplified implementation for now
}

void SemanticAnalyzer::analyzeForInStmt(const std::shared_ptr<ForInStmt>& for_in_stmt) {
    // Simplified implementation for now
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeExpr(const std::shared_ptr<Expr>& expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeBinaryExpr(const std::shared_ptr<BinaryExpr>& binary_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeUnaryExpr(const std::shared_ptr<UnaryExpr>& unary_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeLiteralExpr(const std::shared_ptr<LiteralExpr>& literal_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeIdentifierExpr(const std::shared_ptr<IdentifierExpr>& ident_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeCallExpr(const std::shared_ptr<CallExpr>& call_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeAssignExpr(const std::shared_ptr<AssignExpr>& assign_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeArrayAccessExpr(const std::shared_ptr<ArrayAccessExpr>& array_access_expr) {
    // Simplified implementation for now
    auto void_type = std::make_shared<Type>();
    void_type->kind = Type::VOID;
    void_type->name = "void";
    return void_type;
}

bool SemanticAnalyzer::isTypeCompatible(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const {
    // Simplified implementation for now
    return true;
}

bool SemanticAnalyzer::isTypeEqual(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const {
    // Simplified implementation for now
    return true;
}

void SemanticAnalyzer::error(const Position& pos, const std::string& message) {
    std::cerr << "Semantic error at line " << pos.line << ", column " << pos.column << ": " << message << std::endl;
    throw std::runtime_error("Semantic error");
}

void SemanticAnalyzer::warning(const Position& pos, const std::string& message) {
    std::cerr << "Warning at line " << pos.line << ", column " << pos.column << ": " << message << std::endl;
}

} // namespace tang
