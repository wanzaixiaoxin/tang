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
    try {
        analyzeModule(module);
        std::cout << "Semantic analysis completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Semantic analysis failed: " << e.what() << std::endl;
        throw;
    }
}

void SemanticAnalyzer::analyzeModule(const std::shared_ptr<Module>& module) {
    // 首先分析所有全局变量和函数声明
    for (const auto& global_var : module->global_vars) {
        analyzeVarDecl(global_var);
    }
    
    for (const auto& func : module->functions) {
        // 先注册函数到符号表
        current_scope->addFunction(func->name, func);
    }
    
    // 然后分析函数体
    for (const auto& func : module->functions) {
        analyzeFunction(func);
    }
}

void SemanticAnalyzer::analyzeFunction(const std::shared_ptr<FunctionDecl>& func) {
    // 创建新的作用域
    auto function_scope = std::make_shared<SymbolTable>(current_scope);
    current_scope = function_scope;
    
    // 添加参数到符号表
    for (const auto& param : func->params) {
        current_scope->addVariable(param.first, param.second, false);
    }
    
    // 分析函数体
    for (const auto& stmt : func->body) {
        analyzeStmt(stmt);
    }
    
    // 恢复父作用域
    current_scope = function_scope->parent;
}

void SemanticAnalyzer::analyzeStmt(const std::shared_ptr<Stmt>& stmt) {
    if (!stmt) return;
    
    if (auto var_decl = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        analyzeVarDecl(var_decl);
    } else if (auto return_stmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        // 这里需要知道期望的返回类型，暂时使用void
        auto void_type = std::make_shared<Type>();
        void_type->kind = Type::VOID;
        void_type->name = "void";
        analyzeReturnStmt(return_stmt, void_type);
    } else if (auto if_stmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        analyzeIfStmt(if_stmt);
    } else if (auto while_stmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        analyzeWhileStmt(while_stmt);
    } else if (auto for_stmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        analyzeForStmt(for_stmt);
    } else if (auto for_in_stmt = std::dynamic_pointer_cast<ForInStmt>(stmt)) {
        analyzeForInStmt(for_in_stmt);
    } else if (auto expr_stmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        // 分析表达式语句
        analyzeExpr(expr_stmt->expr);
    }
    // 其他语句类型可以在这里添加
}

void SemanticAnalyzer::analyzeVarDecl(const std::shared_ptr<VarDeclStmt>& var_decl) {
    // 检查变量是否已声明
    if (current_scope->hasVariable(var_decl->name)) {
        error(var_decl->position, "Variable '" + var_decl->name + "' already declared in this scope");
    }
    
    // 分析初始化表达式
    if (var_decl->initializer) {
        auto init_type = analyzeExpr(var_decl->initializer);
        
        // 检查类型兼容性
        if (!isTypeCompatible(var_decl->type, init_type)) {
            error(var_decl->position, "Type mismatch in variable initialization");
        }
    }
    
    // 添加到符号表
    current_scope->addVariable(var_decl->name, var_decl->type, var_decl->is_const);
}

void SemanticAnalyzer::analyzeReturnStmt(const std::shared_ptr<ReturnStmt>& return_stmt, const std::shared_ptr<Type>& expected_type) {
    if (return_stmt->expr) {
        auto expr_type = analyzeExpr(return_stmt->expr);
        
        if (!isTypeCompatible(expected_type, expr_type)) {
            error(return_stmt->position, "Return type mismatch");
        }
    } else {
        // 没有表达式的return语句，检查是否与void兼容
        auto void_type = std::make_shared<Type>();
        void_type->kind = Type::VOID;
        void_type->name = "void";
        
        if (!isTypeCompatible(expected_type, void_type)) {
            error(return_stmt->position, "Void return in non-void function");
        }
    }
}

void SemanticAnalyzer::analyzeIfStmt(const std::shared_ptr<IfStmt>& if_stmt) {
    // 分析条件表达式
    auto condition_type = analyzeExpr(if_stmt->condition);
    
    // 检查条件是否为布尔类型
    auto bool_type = std::make_shared<Type>();
    bool_type->kind = Type::BOOL;
    bool_type->name = "bool";
    
    if (!isTypeCompatible(bool_type, condition_type)) {
        error(if_stmt->condition->position, "If condition must be boolean");
    }
    
    // 分析then分支
    auto then_scope = std::make_shared<SymbolTable>(current_scope);
    current_scope = then_scope;
    for (const auto& stmt : if_stmt->then_branch) {
        analyzeStmt(stmt);
    }
    current_scope = then_scope->parent;
    
    // 分析else分支
    if (!if_stmt->else_branch.empty()) {
        auto else_scope = std::make_shared<SymbolTable>(current_scope);
        current_scope = else_scope;
        for (const auto& stmt : if_stmt->else_branch) {
            analyzeStmt(stmt);
        }
        current_scope = else_scope->parent;
    }
}

void SemanticAnalyzer::analyzeWhileStmt(const std::shared_ptr<WhileStmt>& while_stmt) {
    // 分析条件表达式
    auto condition_type = analyzeExpr(while_stmt->condition);
    
    // 检查条件是否为布尔类型
    auto bool_type = std::make_shared<Type>();
    bool_type->kind = Type::BOOL;
    bool_type->name = "bool";
    
    if (!isTypeCompatible(bool_type, condition_type)) {
        error(while_stmt->condition->position, "While condition must be boolean");
    }
    
    // 分析循环体
    auto loop_scope = std::make_shared<SymbolTable>(current_scope);
    current_scope = loop_scope;
    for (const auto& stmt : while_stmt->body) {
        analyzeStmt(stmt);
    }
    current_scope = loop_scope->parent;
}

void SemanticAnalyzer::analyzeForStmt(const std::shared_ptr<ForStmt>& for_stmt) {
    // 分析初始化语句
    if (for_stmt->init) {
        analyzeStmt(for_stmt->init);
    }
    
    // 分析条件表达式
    if (for_stmt->condition) {
        auto condition_type = analyzeExpr(for_stmt->condition);
        
        auto bool_type = std::make_shared<Type>();
        bool_type->kind = Type::BOOL;
        bool_type->name = "bool";
        
        if (!isTypeCompatible(bool_type, condition_type)) {
            error(for_stmt->condition->position, "For condition must be boolean");
        }
    }
    
    // 分析递增表达式
    if (for_stmt->increment) {
        analyzeExpr(for_stmt->increment);
    }
    
    // 分析循环体
    auto loop_scope = std::make_shared<SymbolTable>(current_scope);
    current_scope = loop_scope;
    for (const auto& stmt : for_stmt->body) {
        analyzeStmt(stmt);
    }
    current_scope = loop_scope->parent;
}

void SemanticAnalyzer::analyzeForInStmt(const std::shared_ptr<ForInStmt>& for_in_stmt) {
    // 分析范围表达式
    auto range_type = analyzeExpr(for_in_stmt->range);
    
    // 检查范围是否为数组类型
    if (range_type->kind != Type::ARRAY) {
        error(for_in_stmt->range->position, "For-in range must be an array");
    }
    
    // 创建循环变量
    auto loop_scope = std::make_shared<SymbolTable>(current_scope);
    current_scope = loop_scope;
    
    // 添加循环变量到符号表
    auto element_type = range_type->element_type;
    current_scope->addVariable(for_in_stmt->var_name, element_type, false);
    
    // 分析循环体
    for (const auto& stmt : for_in_stmt->body) {
        analyzeStmt(stmt);
    }
    current_scope = loop_scope->parent;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeExpr(const std::shared_ptr<Expr>& expr) {
    if (!expr) {
        error(Position{0, 0}, "Null expression");
        return nullptr;
    }
    
    if (auto binary_expr = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        return analyzeBinaryExpr(binary_expr);
    } else if (auto unary_expr = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return analyzeUnaryExpr(unary_expr);
    } else if (auto literal_expr = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        return analyzeLiteralExpr(literal_expr);
    } else if (auto ident_expr = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        return analyzeIdentifierExpr(ident_expr);
    } else if (auto call_expr = std::dynamic_pointer_cast<CallExpr>(expr)) {
        return analyzeCallExpr(call_expr);
    } else if (auto assign_expr = std::dynamic_pointer_cast<AssignExpr>(expr)) {
        return analyzeAssignExpr(assign_expr);
    } else if (auto array_access_expr = std::dynamic_pointer_cast<ArrayAccessExpr>(expr)) {
        return analyzeArrayAccessExpr(array_access_expr);
    }
    
    error(expr->position, "Unsupported expression type");
    return nullptr;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeBinaryExpr(const std::shared_ptr<BinaryExpr>& binary_expr) {
    auto left_type = analyzeExpr(binary_expr->left);
    auto right_type = analyzeExpr(binary_expr->right);
    
    switch (binary_expr->op) {
        case BinaryExpr::ADD:
        case BinaryExpr::SUB:
        case BinaryExpr::MUL:
        case BinaryExpr::DIV:
        case BinaryExpr::MOD:
            // 算术运算：需要数值类型
            if (left_type->kind != Type::INT && left_type->kind != Type::FLOAT) {
                error(binary_expr->left->position, "Left operand must be numeric");
            }
            if (right_type->kind != Type::INT && right_type->kind != Type::FLOAT) {
                error(binary_expr->right->position, "Right operand must be numeric");
            }
            
            // 返回类型：如果两边都是int，返回int；否则返回float
            if (left_type->kind == Type::FLOAT || right_type->kind == Type::FLOAT) {
                auto float_type = std::make_shared<Type>();
                float_type->kind = Type::FLOAT;
                float_type->name = "float";
                return float_type;
            } else {
                auto int_type = std::make_shared<Type>();
                int_type->kind = Type::INT;
                int_type->name = "int";
                return int_type;
            }
            
        case BinaryExpr::EQ:
        case BinaryExpr::NEQ:
        case BinaryExpr::LT:
        case BinaryExpr::LTE:
        case BinaryExpr::GT:
        case BinaryExpr::GTE:
            // 比较运算：需要类型兼容
            if (!isTypeCompatible(left_type, right_type)) {
                error(binary_expr->position, "Type mismatch in comparison");
            }
            
            // 返回布尔类型
            auto bool_type = std::make_shared<Type>();
            bool_type->kind = Type::BOOL;
            bool_type->name = "bool";
            return bool_type;
            
        case BinaryExpr::AND:
        case BinaryExpr::OR:
            // 逻辑运算：需要布尔类型
            auto bool_type = std::make_shared<Type>();
            bool_type->kind = Type::BOOL;
            bool_type->name = "bool";
            
            if (!isTypeCompatible(bool_type, left_type)) {
                error(binary_expr->left->position, "Left operand must be boolean");
            }
            if (!isTypeCompatible(bool_type, right_type)) {
                error(binary_expr->right->position, "Right operand must be boolean");
            }
            return bool_type;
    }
    
    error(binary_expr->position, "Unsupported binary operation");
    return nullptr;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeUnaryExpr(const std::shared_ptr<UnaryExpr>& unary_expr) {
    auto expr_type = analyzeExpr(unary_expr->expr);
    
    switch (unary_expr->op) {
        case UnaryExpr::NOT:
            // 逻辑非：需要布尔类型
            auto bool_type = std::make_shared<Type>();
            bool_type->kind = Type::BOOL;
            bool_type->name = "bool";
            
            if (!isTypeCompatible(bool_type, expr_type)) {
                error(unary_expr->expr->position, "Operand must be boolean for NOT operation");
            }
            return bool_type;
            
        case UnaryExpr::NEG:
            // 算术负号：需要数值类型
            if (expr_type->kind != Type::INT && expr_type->kind != Type::FLOAT) {
                error(unary_expr->expr->position, "Operand must be numeric for negation");
            }
            return expr_type; // 返回相同的类型
    }
    
    error(unary_expr->position, "Unsupported unary operation");
    return nullptr;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeLiteralExpr(const std::shared_ptr<LiteralExpr>& literal_expr) {
    auto type = std::make_shared<Type>();
    
    switch (literal_expr->kind) {
        case LiteralExpr::INT_LITERAL:
            type->kind = Type::INT;
            type->name = "int";
            break;
        case LiteralExpr::FLOAT_LITERAL:
            type->kind = Type::FLOAT;
            type->name = "float";
            break;
        case LiteralExpr::BOOL_LITERAL:
            type->kind = Type::BOOL;
            type->name = "bool";
            break;
        case LiteralExpr::STRING_LITERAL:
            type->kind = Type::STRING;
            type->name = "string";
            break;
        default:
            error(literal_expr->position, "Unsupported literal type");
            return nullptr;
    }
    
    return type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeIdentifierExpr(const std::shared_ptr<IdentifierExpr>& ident_expr) {
    // 查找变量或函数
    auto var_type = current_scope->getVariableType(ident_expr->name);
    if (var_type) {
        return var_type;
    }
    
    // 检查是否为函数
    auto func = current_scope->getFunction(ident_expr->name);
    if (func) {
        // 创建函数类型
        auto func_type = std::make_shared<Type>();
        func_type->kind = Type::FUNCTION;
        func_type->return_type = func->return_type;
        func_type->name = func->name;
        
        // 添加参数类型
        for (const auto& param : func->params) {
            func_type->param_types.push_back(param.second);
        }
        
        return func_type;
    }
    
    error(ident_expr->position, "Undefined identifier: " + ident_expr->name);
    return nullptr;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeCallExpr(const std::shared_ptr<CallExpr>& call_expr) {
    // 分析被调用表达式
    auto callee_type = analyzeExpr(call_expr->callee);
    
    if (callee_type->kind != Type::FUNCTION) {
        error(call_expr->callee->position, "Can only call functions");
        return nullptr;
    }
    
    // 检查参数数量
    if (call_expr->args.size() != callee_type->param_types.size()) {
        error(call_expr->position, "Argument count mismatch");
        return nullptr;
    }
    
    // 检查参数类型
    for (size_t i = 0; i < call_expr->args.size(); i++) {
        auto arg_type = analyzeExpr(call_expr->args[i]);
        if (!isTypeCompatible(callee_type->param_types[i], arg_type)) {
            error(call_expr->args[i]->position, "Argument type mismatch");
        }
    }
    
    return callee_type->return_type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeAssignExpr(const std::shared_ptr<AssignExpr>& assign_expr) {
    // 分析目标（必须是标识符）
    auto target_type = analyzeExpr(assign_expr->target);
    
    // 检查目标是否为变量（不是常量）
    if (auto ident_expr = std::dynamic_pointer_cast<IdentifierExpr>(assign_expr->target)) {
        if (current_scope->isVariableConst(ident_expr->name)) {
            error(ident_expr->position, "Cannot assign to constant variable");
        }
    } else {
        error(assign_expr->target->position, "Assignment target must be a variable");
    }
    
    // 分析值
    auto value_type = analyzeExpr(assign_expr->value);
    
    // 检查类型兼容性
    if (!isTypeCompatible(target_type, value_type)) {
        error(assign_expr->value->position, "Type mismatch in assignment");
    }
    
    return target_type; // 赋值表达式返回目标类型
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeArrayAccessExpr(const std::shared_ptr<ArrayAccessExpr>& array_access_expr) {
    // 分析数组表达式
    auto array_type = analyzeExpr(array_access_expr->array);
    
    if (array_type->kind != Type::ARRAY) {
        error(array_access_expr->array->position, "Array access requires array type");
        return nullptr;
    }
    
    // 分析索引表达式
    auto index_type = analyzeExpr(array_access_expr->index);
    
    auto int_type = std::make_shared<Type>();
    int_type->kind = Type::INT;
    int_type->name = "int";
    
    if (!isTypeCompatible(int_type, index_type)) {
        error(array_access_expr->index->position, "Array index must be integer");
    }
    
    return array_type->element_type; // 返回数组元素类型
}

bool SemanticAnalyzer::isTypeCompatible(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const {
    if (!lhs || !rhs) {
        return false;
    }
    
    // 基本类型兼容性规则
    if (lhs->kind == rhs->kind) {
        switch (lhs->kind) {
            case Type::INT:
            case Type::FLOAT:
            case Type::BOOL:
            case Type::STRING:
            case Type::VOID:
                return true;
            case Type::ARRAY:
                return isTypeCompatible(lhs->element_type, rhs->element_type);
            case Type::FUNCTION:
                // 检查参数类型和返回类型
                if (lhs->param_types.size() != rhs->param_types.size()) {
                    return false;
                }
                for (size_t i = 0; i < lhs->param_types.size(); i++) {
                    if (!isTypeCompatible(lhs->param_types[i], rhs->param_types[i])) {
                        return false;
                    }
                }
                return isTypeCompatible(lhs->return_type, rhs->return_type);
            case Type::RESULT:
                return isTypeCompatible(lhs->ok_type, rhs->ok_type) && 
                       isTypeCompatible(lhs->err_type, rhs->err_type);
            case Type::CUSTOM:
                return lhs->name == rhs->name; // 自定义类型按名称匹配
        }
    }
    
    // 数值类型隐式转换
    if (lhs->kind == Type::INT && rhs->kind == Type::FLOAT) {
        return true; // int 可以隐式转换为 float
    }
    
    return false;
}

bool SemanticAnalyzer::isTypeEqual(const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs) const {
    if (!lhs || !rhs) {
        return lhs == rhs; // 都为nullptr时相等
    }
    
    if (lhs->kind != rhs->kind) {
        return false;
    }
    
    switch (lhs->kind) {
        case Type::INT:
        case Type::FLOAT:
        case Type::BOOL:
        case Type::STRING:
        case Type::VOID:
            return true; // 基本类型总是相等
        case Type::ARRAY:
            return isTypeEqual(lhs->element_type, rhs->element_type);
        case Type::FUNCTION:
            if (lhs->param_types.size() != rhs->param_types.size()) {
                return false;
            }
            for (size_t i = 0; i < lhs->param_types.size(); i++) {
                if (!isTypeEqual(lhs->param_types[i], rhs->param_types[i])) {
                    return false;
                }
            }
            return isTypeEqual(lhs->return_type, rhs->return_type);
        case Type::RESULT:
            return isTypeEqual(lhs->ok_type, rhs->ok_type) && 
                   isTypeEqual(lhs->err_type, rhs->err_type);
        case Type::CUSTOM:
            return lhs->name == rhs->name;
    }
    
    return false;
}

void SemanticAnalyzer::error(const Position& pos, const std::string& message) {
    std::cerr << "Semantic error at line " << pos.line << ", column " << pos.column << ": " << message << std::endl;
    throw std::runtime_error("Semantic error");
}

void SemanticAnalyzer::warning(const Position& pos, const std::string& message) {
    std::cerr << "Warning at line " << pos.line << ", column " << pos.column << ": " << message << std::endl;
}

} // namespace tang
