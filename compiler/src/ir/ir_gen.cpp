#include <iostream>
#include <memory>
#include <vector>
#include "../../include/ast.h"
#include "../../include/ir.h"

namespace tang {
namespace ir {

IRGenerator::IRGenerator() {
    // Initialize
}

Module IRGenerator::createModule() {
    Module module;
    module.next_func_id = 0;
    module.next_bb_id = 0;
    module.next_reg_id = 0;
    return module;
}

FunctionId IRGenerator::createFunction(Module& module, const std::string& name, FunctionType type) {
    Function func;
    func.id = module.next_func_id++;
    func.name = name;
    func.type = type;
    func.entry_block = -1;
    func.exit_block = -1;
    module.functions.push_back(func);
    return func.id;
}

void IRGenerator::addParameter(Module& module, FunctionId func_id, Register reg) {
    Function& func = getFunction(module, func_id);
    func.params.push_back(reg);
}

BasicBlockId IRGenerator::createBasicBlock(Module& module, FunctionId func_id) {
    Function& func = getFunction(module, func_id);
    BasicBlock bb;
    bb.id = module.next_bb_id++;
    func.basic_blocks.push_back(bb);
    return bb.id;
}

void IRGenerator::setEntryBlock(Module& module, FunctionId func_id, BasicBlockId bb_id) {
    Function& func = getFunction(module, func_id);
    func.entry_block = bb_id;
}

void IRGenerator::setExitBlock(Module& module, FunctionId func_id, BasicBlockId bb_id) {
    Function& func = getFunction(module, func_id);
    func.exit_block = bb_id;
}

void IRGenerator::addInstruction(Module& module, FunctionId func_id, BasicBlockId bb_id, const Instruction& instr) {
    BasicBlock& bb = getBasicBlock(module, func_id, bb_id);
    bb.instructions.push_back(instr);
}

Register IRGenerator::allocateRegister(Module& module) {
    return module.next_reg_id++;
}

Instruction IRGenerator::createInstruction(OpCode op_code, Register dst, const std::vector<Operand>& operands) {
    Instruction instr;
    instr.op_code = op_code;
    instr.dst = dst;
    instr.operands = operands;
    return instr;
}

Operand IRGenerator::createRegisterOperand(Register reg) {
    Operand op;
    op.type = REGISTER;
    op.value.reg = reg;
    return op;
}

Operand IRGenerator::createImmediateOperand(int32_t imm) {
    Operand op;
    op.type = IMMEDIATE;
    op.value.imm = imm;
    return op;
}

Operand IRGenerator::createBasicBlockOperand(BasicBlockId bb) {
    Operand op;
    op.type = BASIC_BLOCK;
    op.value.bb = bb;
    return op;
}

Operand IRGenerator::createFunctionOperand(FunctionId func) {
    Operand op;
    op.type = FUNCTION;
    op.value.func = func;
    return op;
}

Operand IRGenerator::createMemoryOperand(Register base, int32_t offset) {
    Operand op;
    op.type = MEMORY;
    op.value.mem.base = base;
    op.value.mem.offset = offset;
    return op;
}

Function& IRGenerator::getFunction(Module& module, FunctionId func_id) {
    for (size_t i = 0; i < module.functions.size(); i++) {
        if (module.functions[i].id == func_id) {
            return module.functions[i];
        }
    }
    throw std::runtime_error("Function not found");
}

BasicBlock& IRGenerator::getBasicBlock(Module& module, FunctionId func_id, BasicBlockId bb_id) {
    Function& func = getFunction(module, func_id);
    for (size_t i = 0; i < func.basic_blocks.size(); i++) {
        if (func.basic_blocks[i].id == bb_id) {
            return func.basic_blocks[i];
        }
    }
    throw std::runtime_error("Basic block not found");
}

// IROptimizer implementation
IROptimizer::IROptimizer() {
    // Initialize
}

void IROptimizer::optimize(Module& module) {
    for (size_t i = 0; i < module.functions.size(); i++) {
        optimizeFunction(module, module.functions[i]);
    }
}

void IROptimizer::optimizeFunction(Module& module, Function& func) {
    // Run various optimization passes
    constantFolding(module, func);
    deadCodeElimination(module, func);
    peepholeOptimization(module, func);
}

void IROptimizer::constantFolding(Module& module, Function& func) {
    // Constant folding optimization
    // Simplified processing, actual implementation needs more complex logic
    for (size_t i = 0; i < func.basic_blocks.size(); i++) {
        BasicBlock& bb = func.basic_blocks[i];
        for (size_t j = 0; j < bb.instructions.size(); j++) {
            Instruction& instr = bb.instructions[j];
            // Example: fold constant addition
            if (instr.op_code == OP_ADD) {
                if (instr.operands.size() == 2) {
                    if (instr.operands[0].type == IMMEDIATE && instr.operands[1].type == IMMEDIATE) {
                        int32_t result = instr.operands[0].value.imm + instr.operands[1].value.imm;
                        instr.op_code = OP_CONST;
                        instr.operands.clear();
                        Operand imm_op;
                        imm_op.type = IMMEDIATE;
                        imm_op.value.imm = result;
                        instr.operands.push_back(imm_op);
                    }
                }
            }
        }
    }
}

void IROptimizer::deadCodeElimination(Module& module, Function& func) {
    // Dead code elimination
    // Simplified processing, actual implementation needs more complex data flow analysis
}

void IROptimizer::peepholeOptimization(Module& module, Function& func) {
    // Peephole optimization
    // Simplified processing, actual implementation needs more complex pattern matching
}

// IR generation context
struct IRGenContext {
    IRGenerator& generator;
    Module& module;
    FunctionId current_func_id;
    BasicBlockId current_bb;
    std::unordered_map<std::string, Register> variable_map;
    
    IRGenContext(IRGenerator& gen, Module& mod, FunctionId func_id, BasicBlockId bb_id)
        : generator(gen), module(mod), current_func_id(func_id), current_bb(bb_id) {}
};

// Expression to IR conversion
Register generateExprIR(IRGenContext& context, const std::shared_ptr<tang::Expr>& expr);

// Statement to IR conversion
void generateStmtIR(IRGenContext& context, const std::shared_ptr<tang::Stmt>& stmt);

// Function to IR conversion
void generateFunctionIR(IRGenContext& context, const std::shared_ptr<tang::FunctionDecl>& func_decl);

// Async function to IR conversion (state machine)
void generateAsyncFunctionIR(IRGenContext& context, const std::shared_ptr<tang::FunctionDecl>& func_decl);

// AST to IR conversion function
Module generateIR(const std::shared_ptr<tang::Module>& ast_module) {
    IRGenerator generator;
    Module ir_module = generator.createModule();
    
    // Traverse AST module, generate IR
    for (size_t i = 0; i < ast_module->functions.size(); i++) {
        const std::shared_ptr<tang::FunctionDecl>& func_decl = ast_module->functions[i];
        
        // Determine function type
        FunctionType func_type = func_decl->is_sync ? SYNC : ASYNC;
        
        // Create IR function
        FunctionId func_id = generator.createFunction(ir_module, func_decl->name, func_type);
        
        // Create entry basic block
        BasicBlockId entry_bb = generator.createBasicBlock(ir_module, func_id);
        generator.setEntryBlock(ir_module, func_id, entry_bb);
        
        // Create IR generation context
        IRGenContext context(generator, ir_module, func_id, entry_bb);
        
        // Generate function IR
        generateFunctionIR(context, func_decl);
    }
    
    return ir_module;
}

void generateFunctionIR(IRGenContext& context, const std::shared_ptr<tang::FunctionDecl>& func_decl) {
    // Allocate parameter registers and add to variable map
    for (size_t i = 0; i < func_decl->params.size(); i++) {
        const auto& param = func_decl->params[i];
        const std::string& param_name = param.first;
        
        Register reg = context.generator.allocateRegister(context.module);
        context.generator.addParameter(context.module, context.current_func_id, reg);
        context.variable_map[param_name] = reg;
        
        // Generate parameter load instruction
        std::vector<Operand> load_ops;
        Operand mem_op = context.generator.createMemoryOperand(reg, 0);
        load_ops.push_back(mem_op);
        
        Instruction load_instr = context.generator.createInstruction(OP_LOAD, reg, load_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, load_instr);
    }
    
    // For async functions, generate state machine structure
    if (!func_decl->is_sync) {
        generateAsyncFunctionIR(context, func_decl);
        return;
    }
    
    // Generate function body for sync functions
    for (const auto& stmt : func_decl->body) {
        generateStmtIR(context, stmt);
    }
    
    // Add return instruction if not present
    if (func_decl->body.empty() || 
        !std::dynamic_pointer_cast<tang::ReturnStmt>(func_decl->body.back())) {
        Instruction ret_instr = context.generator.createInstruction(OP_RET, -1, {});
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, ret_instr);
    }
}

void generateStmtIR(IRGenContext& context, const std::shared_ptr<tang::Stmt>& stmt) {
    if (!stmt) return;
    
    if (auto var_decl = std::dynamic_pointer_cast<tang::VarDeclStmt>(stmt)) {
        // Variable declaration
        Register reg = context.generator.allocateRegister(context.module);
        context.variable_map[var_decl->name] = reg;
        
        if (var_decl->initializer) {
            // Generate initialization expression
            Register init_reg = generateExprIR(context, var_decl->initializer);
            
            // Generate store instruction
            std::vector<Operand> store_ops;
            Operand mem_op = context.generator.createMemoryOperand(reg, 0);
            Operand value_op = context.generator.createRegisterOperand(init_reg);
            store_ops.push_back(mem_op);
            store_ops.push_back(value_op);
            
            Instruction store_instr = context.generator.createInstruction(OP_STORE, -1, store_ops);
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, store_instr);
        }
        
    } else if (auto return_stmt = std::dynamic_pointer_cast<tang::ReturnStmt>(stmt)) {
        if (return_stmt->expr) {
            // Generate return value expression
            Register ret_reg = generateExprIR(context, return_stmt->expr);
            
            std::vector<Operand> ret_ops;
            Operand ret_val_op = context.generator.createRegisterOperand(ret_reg);
            ret_ops.push_back(ret_val_op);
            
            Instruction ret_instr = context.generator.createInstruction(OP_RET, -1, ret_ops);
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, ret_instr);
        } else {
            // Void return
            Instruction ret_instr = context.generator.createInstruction(OP_RET, -1, {});
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, ret_instr);
        }
        
    } else if (auto expr_stmt = std::dynamic_pointer_cast<tang::ExprStmt>(stmt)) {
        // Expression statement - just evaluate the expression
        generateExprIR(context, expr_stmt->expr);
        
    } else if (auto if_stmt = std::dynamic_pointer_cast<tang::IfStmt>(stmt)) {
        // If statement
        Register cond_reg = generateExprIR(context, if_stmt->condition);
        
        // Create then and else blocks
        BasicBlockId then_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        BasicBlockId else_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        BasicBlockId merge_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        
        // Generate conditional branch
        std::vector<Operand> branch_ops;
        Operand cond_op = context.generator.createRegisterOperand(cond_reg);
        Operand then_op = context.generator.createBasicBlockOperand(then_bb);
        Operand else_op = context.generator.createBasicBlockOperand(else_bb);
        branch_ops.push_back(cond_op);
        branch_ops.push_back(then_op);
        branch_ops.push_back(else_op);
        
        Instruction branch_instr = context.generator.createInstruction(OP_BRANCH, -1, branch_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, branch_instr);
        
        // Generate then branch
        context.current_bb = then_bb;
        for (const auto& then_stmt : if_stmt->then_branch) {
            generateStmtIR(context, then_stmt);
        }
        
        // Jump to merge block
        std::vector<Operand> jmp_ops;
        Operand merge_op = context.generator.createBasicBlockOperand(merge_bb);
        jmp_ops.push_back(merge_op);
        
        Instruction jmp_instr = context.generator.createInstruction(OP_JMP, -1, jmp_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, jmp_instr);
        
        // Generate else branch
        context.current_bb = else_bb;
        for (const auto& else_stmt : if_stmt->else_branch) {
            generateStmtIR(context, else_stmt);
        }
        
        // Jump to merge block
        Instruction jmp_instr2 = context.generator.createInstruction(OP_JMP, -1, jmp_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, jmp_instr2);
        
        // Continue with merge block
        context.current_bb = merge_bb;
        
    } else if (auto while_stmt = std::dynamic_pointer_cast<tang::WhileStmt>(stmt)) {
        // While statement
        BasicBlockId cond_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        BasicBlockId body_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        BasicBlockId exit_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        
        // Jump to condition block
        std::vector<Operand> jmp_ops;
        Operand cond_op = context.generator.createBasicBlockOperand(cond_bb);
        jmp_ops.push_back(cond_op);
        
        Instruction jmp_instr = context.generator.createInstruction(OP_JMP, -1, jmp_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, jmp_instr);
        
        // Generate condition block
        context.current_bb = cond_bb;
        Register cond_reg = generateExprIR(context, while_stmt->condition);
        
        // Conditional branch
        std::vector<Operand> branch_ops;
        Operand cond_val_op = context.generator.createRegisterOperand(cond_reg);
        Operand body_op = context.generator.createBasicBlockOperand(body_bb);
        Operand exit_op = context.generator.createBasicBlockOperand(exit_bb);
        branch_ops.push_back(cond_val_op);
        branch_ops.push_back(body_op);
        branch_ops.push_back(exit_op);
        
        Instruction branch_instr = context.generator.createInstruction(OP_BRANCH, -1, branch_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, branch_instr);
        
        // Generate body block
        context.current_bb = body_bb;
        for (const auto& body_stmt : while_stmt->body) {
            generateStmtIR(context, body_stmt);
        }
        
        // Jump back to condition
        std::vector<Operand> jmp_back_ops;
        jmp_back_ops.push_back(cond_op);
        
        Instruction jmp_back_instr = context.generator.createInstruction(OP_JMP, -1, jmp_back_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, jmp_back_instr);
        
        // Continue with exit block
        context.current_bb = exit_bb;
        
    } else if (auto yield_stmt = std::dynamic_pointer_cast<tang::YieldStmt>(stmt)) {
        // Yield statement (coroutine yield)
        if (yield_stmt->expr) {
            Register yield_reg = generateExprIR(context, yield_stmt->expr);
            
            std::vector<Operand> yield_ops;
            Operand value_op = context.generator.createRegisterOperand(yield_reg);
            yield_ops.push_back(value_op);
            
            Instruction yield_instr = context.generator.createInstruction(OP_CORO_YIELD, -1, yield_ops);
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, yield_instr);
        } else {
            // Void yield
            Instruction yield_instr = context.generator.createInstruction(OP_CORO_YIELD, -1, {});
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, yield_instr);
        }
        
    } else if (auto coro_create = std::dynamic_pointer_cast<tang::CoroCreateStmt>(stmt)) {
        // Coroutine creation statement
        Register func_reg = generateExprIR(context, coro_create->func_expr);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        std::vector<Operand> create_ops;
        Operand func_op = context.generator.createRegisterOperand(func_reg);
        create_ops.push_back(func_op);
        
        Instruction create_instr = context.generator.createInstruction(OP_CORO_CREATE, result_reg, create_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, create_instr);
        
        // Store coroutine handle if needed
        if (!coro_create->var_name.empty()) {
            context.variable_map[coro_create->var_name] = result_reg;
        }
        
    } else if (auto coro_resume = std::dynamic_pointer_cast<tang::CoroResumeStmt>(stmt)) {
        // Coroutine resume statement
        Register coro_reg = generateExprIR(context, coro_resume->coro_expr);
        
        std::vector<Operand> resume_ops;
        Operand coro_op = context.generator.createRegisterOperand(coro_reg);
        resume_ops.push_back(coro_op);
        
        Instruction resume_instr = context.generator.createInstruction(OP_CORO_RESUME, -1, resume_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, resume_instr);
        
    } else if (auto coro_destroy = std::dynamic_pointer_cast<tang::CoroDestroyStmt>(stmt)) {
        // Coroutine destroy statement
        Register coro_reg = generateExprIR(context, coro_destroy->coro_expr);
        
        std::vector<Operand> destroy_ops;
        Operand coro_op = context.generator.createRegisterOperand(coro_reg);
        destroy_ops.push_back(coro_op);
        
        Instruction destroy_instr = context.generator.createInstruction(OP_CORO_DESTROY, -1, destroy_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, destroy_instr);
    }
    // Other statement types can be added here
}

Register generateExprIR(IRGenContext& context, const std::shared_ptr<tang::Expr>& expr) {
    if (!expr) {
        throw std::runtime_error("Null expression in IR generation");
    }
    
    if (auto binary_expr = std::dynamic_pointer_cast<tang::BinaryExpr>(expr)) {
        // Binary expression
        Register left_reg = generateExprIR(context, binary_expr->left);
        Register right_reg = generateExprIR(context, binary_expr->right);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        std::vector<Operand> op_ops;
        Operand left_op = context.generator.createRegisterOperand(left_reg);
        Operand right_op = context.generator.createRegisterOperand(right_reg);
        op_ops.push_back(left_op);
        op_ops.push_back(right_op);
        
        OpCode op_code;
        switch (binary_expr->op) {
            case tang::BinaryExpr::ADD: op_code = OP_ADD; break;
            case tang::BinaryExpr::SUB: op_code = OP_SUB; break;
            case tang::BinaryExpr::MUL: op_code = OP_MUL; break;
            case tang::BinaryExpr::DIV: op_code = OP_DIV; break;
            case tang::BinaryExpr::MOD: op_code = OP_MOD; break;
            case tang::BinaryExpr::EQ: op_code = OP_EQ; break;
            case tang::BinaryExpr::NEQ: op_code = OP_NEQ; break;
            case tang::BinaryExpr::LT: op_code = OP_LT; break;
            case tang::BinaryExpr::LTE: op_code = OP_LTE; break;
            case tang::BinaryExpr::GT: op_code = OP_GT; break;
            case tang::BinaryExpr::GTE: op_code = OP_GTE; break;
            case tang::BinaryExpr::AND: op_code = OP_AND; break;
            case tang::BinaryExpr::OR: op_code = OP_OR; break;
            default: throw std::runtime_error("Unsupported binary operation");
        }
        
        Instruction instr = context.generator.createInstruction(op_code, result_reg, op_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, instr);
        
        return result_reg;
        
    } else if (auto unary_expr = std::dynamic_pointer_cast<tang::UnaryExpr>(expr)) {
        // Unary expression
        Register expr_reg = generateExprIR(context, unary_expr->expr);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        std::vector<Operand> op_ops;
        Operand expr_op = context.generator.createRegisterOperand(expr_reg);
        op_ops.push_back(expr_op);
        
        OpCode op_code;
        switch (unary_expr->op) {
            case tang::UnaryExpr::NOT: op_code = OP_NOT; break;
            case tang::UnaryExpr::NEG: op_code = OP_SUB; break; // Use subtraction from zero
            default: throw std::runtime_error("Unsupported unary operation");
        }
        
        Instruction instr = context.generator.createInstruction(op_code, result_reg, op_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, instr);
        
        return result_reg;
        
    } else if (auto literal_expr = std::dynamic_pointer_cast<tang::LiteralExpr>(expr)) {
        // Literal expression
        Register result_reg = context.generator.allocateRegister(context.module);
        
        int32_t value = 0;
        if (literal_expr->kind == tang::LiteralExpr::INT_LITERAL) {
            value = std::stoi(literal_expr->value);
        } else if (literal_expr->kind == tang::LiteralExpr::BOOL_LITERAL) {
            value = (literal_expr->value == "true") ? 1 : 0;
        }
        
        std::vector<Operand> const_ops;
        Operand imm_op = context.generator.createImmediateOperand(value);
        const_ops.push_back(imm_op);
        
        Instruction instr = context.generator.createInstruction(OP_CONST, result_reg, const_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, instr);
        
        return result_reg;
        
    } else if (auto ident_expr = std::dynamic_pointer_cast<tang::IdentifierExpr>(expr)) {
        // Identifier expression
        auto it = context.variable_map.find(ident_expr->name);
        if (it == context.variable_map.end()) {
            throw std::runtime_error("Undefined variable: " + ident_expr->name);
        }
        
        Register var_reg = it->second;
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Load variable value
        std::vector<Operand> load_ops;
        Operand mem_op = context.generator.createMemoryOperand(var_reg, 0);
        load_ops.push_back(mem_op);
        
        Instruction load_instr = context.generator.createInstruction(OP_LOAD, result_reg, load_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, load_instr);
        
        return result_reg;
        
    } else if (auto call_expr = std::dynamic_pointer_cast<tang::CallExpr>(expr)) {
        // Function call expression
        // Generate arguments
        std::vector<Register> arg_regs;
        for (const auto& arg : call_expr->args) {
            arg_regs.push_back(generateExprIR(context, arg));
        }
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Generate call instruction
        std::vector<Operand> call_ops;
        
        // Generate callee expression
        Register callee_reg = generateExprIR(context, call_expr->callee);
        Operand callee_op = context.generator.createRegisterOperand(callee_reg);
        call_ops.push_back(callee_op);
        
        // Add arguments
        for (Register arg_reg : arg_regs) {
            Operand arg_op = context.generator.createRegisterOperand(arg_reg);
            call_ops.push_back(arg_op);
        }
        
        Instruction call_instr = context.generator.createInstruction(OP_CALL, result_reg, call_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, call_instr);
        
        return result_reg;
        
    } else if (auto assign_expr = std::dynamic_pointer_cast<tang::AssignExpr>(expr)) {
        // Assignment expression
        Register value_reg = generateExprIR(context, assign_expr->value);
        
        // Find target variable
        if (auto target_ident = std::dynamic_pointer_cast<tang::IdentifierExpr>(assign_expr->target)) {
            auto it = context.variable_map.find(target_ident->name);
            if (it == context.variable_map.end()) {
                throw std::runtime_error("Undefined variable: " + target_ident->name);
            }
            
            Register target_reg = it->second;
            
            // Generate store instruction
            std::vector<Operand> store_ops;
            Operand mem_op = context.generator.createMemoryOperand(target_reg, 0);
            Operand value_op = context.generator.createRegisterOperand(value_reg);
            store_ops.push_back(mem_op);
            store_ops.push_back(value_op);
            
            Instruction store_instr = context.generator.createInstruction(OP_STORE, -1, store_ops);
            context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, store_instr);
            
            return value_reg; // Assignment expression returns the assigned value
        }
        
        throw std::runtime_error("Invalid assignment target");
        
    } else if (auto array_access = std::dynamic_pointer_cast<tang::ArrayAccessExpr>(expr)) {
        // Array access expression
        Register array_reg = generateExprIR(context, array_access->array);
        Register index_reg = generateExprIR(context, array_access->index);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Calculate element offset (assuming 4-byte integers)
        Register offset_reg = context.generator.allocateRegister(context.module);
        std::vector<Operand> mul_ops;
        Operand index_op = context.generator.createRegisterOperand(index_reg);
        Operand size_op = context.generator.createImmediateOperand(4); // sizeof(int)
        mul_ops.push_back(index_op);
        mul_ops.push_back(size_op);
        
        Instruction mul_instr = context.generator.createInstruction(OP_MUL, offset_reg, mul_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, mul_instr);
        
        // Load array element
        std::vector<Operand> load_ops;
        Operand mem_op = context.generator.createMemoryOperand(array_reg, offset_reg);
        load_ops.push_back(mem_op);
        
        Instruction load_instr = context.generator.createInstruction(OP_LOAD, result_reg, load_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, load_instr);
        
        return result_reg;
        
    } else if (auto member_access = std::dynamic_pointer_cast<tang::MemberAccessExpr>(expr)) {
        // Member access expression
        Register struct_reg = generateExprIR(context, member_access->object);
        
        // For simplicity, assume fixed struct layout
        // In real implementation, this would use type information
        int offset = 0; // Simplified offset calculation
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Load member value
        std::vector<Operand> load_ops;
        Operand mem_op = context.generator.createMemoryOperand(struct_reg, offset);
        load_ops.push_back(mem_op);
        
        Instruction load_instr = context.generator.createInstruction(OP_LOAD, result_reg, load_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, load_instr);
        
        return result_reg;
        
    } else if (auto new_expr = std::dynamic_pointer_cast<tang::NewExpr>(expr)) {
        // New expression (memory allocation)
        Register size_reg = generateExprIR(context, new_expr->size);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Generate allocation instruction
        std::vector<Operand> alloc_ops;
        Operand size_op = context.generator.createRegisterOperand(size_reg);
        alloc_ops.push_back(size_op);
        
        Instruction alloc_instr = context.generator.createInstruction(OP_ALLOC, result_reg, alloc_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, alloc_instr);
        
        return result_reg;
        
    } else if (auto await_expr = std::dynamic_pointer_cast<tang::AwaitExpr>(expr)) {
        // Await expression (async/await)
        Register future_reg = generateExprIR(context, await_expr->future);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        // Generate async wait instruction
        std::vector<Operand> wait_ops;
        Operand future_op = context.generator.createRegisterOperand(future_reg);
        wait_ops.push_back(future_op);
        
        Instruction wait_instr = context.generator.createInstruction(OP_ASYNC_WAIT, result_reg, wait_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, wait_instr);
        
        return result_reg;
        
    } else if (auto async_read = std::dynamic_pointer_cast<tang::AsyncReadExpr>(expr)) {
        // Async read expression
        Register fd_reg = generateExprIR(context, async_read->fd);
        Register buf_reg = generateExprIR(context, async_read->buf);
        Register size_reg = generateExprIR(context, async_read->size);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        std::vector<Operand> read_ops;
        Operand fd_op = context.generator.createRegisterOperand(fd_reg);
        Operand buf_op = context.generator.createRegisterOperand(buf_reg);
        Operand size_op = context.generator.createRegisterOperand(size_reg);
        read_ops.push_back(fd_op);
        read_ops.push_back(buf_op);
        read_ops.push_back(size_op);
        
        Instruction read_instr = context.generator.createInstruction(OP_ASYNC_READ, result_reg, read_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, read_instr);
        
        return result_reg;
        
    } else if (auto async_write = std::dynamic_pointer_cast<tang::AsyncWriteExpr>(expr)) {
        // Async write expression
        Register fd_reg = generateExprIR(context, async_write->fd);
        Register buf_reg = generateExprIR(context, async_write->buf);
        Register size_reg = generateExprIR(context, async_write->size);
        
        Register result_reg = context.generator.allocateRegister(context.module);
        
        std::vector<Operand> write_ops;
        Operand fd_op = context.generator.createRegisterOperand(fd_reg);
        Operand buf_op = context.generator.createRegisterOperand(buf_reg);
        Operand size_op = context.generator.createRegisterOperand(size_reg);
        write_ops.push_back(fd_op);
        write_ops.push_back(buf_op);
        write_ops.push_back(size_op);
        
        Instruction write_instr = context.generator.createInstruction(OP_ASYNC_WRITE, result_reg, write_ops);
        context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, write_instr);
        
        return result_reg;
    }
    
    throw std::runtime_error("Unsupported expression type in IR generation");
}

// Print IR for debugging
void printIR(const Module& module) {
    std::cout << "IR Module:" << std::endl;
    
    for (size_t i = 0; i < module.functions.size(); i++) {
        const Function& func = module.functions[i];
        std::cout << "\nFunction: " << func.name << " (" << (func.type == SYNC ? "sync" : "async") << ")" << std::endl;
        std::cout << "Entry block: " << func.entry_block << ", Exit block: " << func.exit_block << std::endl;
        
        for (size_t j = 0; j < func.basic_blocks.size(); j++) {
            const BasicBlock& bb = func.basic_blocks[j];
            std::cout << "\nBasic Block " << bb.id << ":" << std::endl;
            
            for (size_t k = 0; k < bb.instructions.size(); k++) {
                const Instruction& instr = bb.instructions[k];
                std::cout << "  ";
                if (instr.dst != -1) {
                    std::cout << "r" << instr.dst << " = ";
                }
                
                // Print opcode
                switch (instr.op_code) {
                    case OP_ADD: std::cout << "add";
                        break;
                    case OP_SUB: std::cout << "sub";
                        break;
                    case OP_MUL: std::cout << "mul";
                        break;
                    case OP_DIV: std::cout << "div";
                        break;
                    case OP_MOD: std::cout << "mod";
                        break;
                    case OP_EQ: std::cout << "eq";
                        break;
                    case OP_NEQ: std::cout << "neq";
                        break;
                    case OP_LT: std::cout << "lt";
                        break;
                    case OP_LTE: std::cout << "lte";
                        break;
                    case OP_GT: std::cout << "gt";
                        break;
                    case OP_GTE: std::cout << "gte";
                        break;
                    case OP_AND: std::cout << "and";
                        break;
                    case OP_OR: std::cout << "or";
                        break;
                    case OP_NOT: std::cout << "not";
                        break;
                    case OP_LOAD: std::cout << "load";
                        break;
                    case OP_STORE: std::cout << "store";
                        break;
                    case OP_ALLOC: std::cout << "alloc";
                        break;
                    case OP_FREE: std::cout << "free";
                        break;
                    case OP_JMP: std::cout << "jmp";
                        break;
                    case OP_BRANCH: std::cout << "branch";
                        break;
                    case OP_CALL: std::cout << "call";
                        break;
                    case OP_RET: std::cout << "ret";
                        break;
                    case OP_CONST: std::cout << "const";
                        break;
                    case OP_CORO_CREATE: std::cout << "coro_create";
                        break;
                    case OP_CORO_YIELD: std::cout << "coro_yield";
                        break;
                    case OP_CORO_RESUME: std::cout << "coro_resume";
                        break;
                    case OP_CORO_DESTROY: std::cout << "coro_destroy";
                        break;
                    case OP_ASYNC_READ: std::cout << "async_read";
                        break;
                    case OP_ASYNC_WRITE: std::cout << "async_write";
                        break;
                    case OP_ASYNC_WAIT: std::cout << "async_wait";
                        break;
                    default: std::cout << "unknown";
                        break;
                }
                
                // Print operands
                if (!instr.operands.empty()) {
                    std::cout << " ";
                    for (size_t m = 0; m < instr.operands.size(); ++m) {
                        if (m > 0) {
                            std::cout << ", ";
                        }
                        
                        const Operand& op = instr.operands[m];
                        switch (op.type) {
                            case REGISTER:
                                std::cout << "r" << op.value.reg;
                                break;
                            case IMMEDIATE:
                                std::cout << op.value.imm;
                                break;
                            case BASIC_BLOCK:
                                std::cout << "bb" << op.value.bb;
                                break;
                            case FUNCTION:
                                std::cout << "func" << op.value.func;
                                break;
                            case MEMORY:
                                std::cout << "mem[r" << op.value.mem.base << "+" << op.value.mem.offset << "]";
                                break;
                        }
                    }
                }
                
                std::cout << std::endl;
            }
        }
    }
}

void generateAsyncFunctionIR(IRGenContext& context, const std::shared_ptr<tang::FunctionDecl>& func_decl) {
    // Async functions are compiled as state machines
    // They use coroutine operations for suspension and resumption
    
    // Create state variable
    Register state_reg = context.generator.allocateRegister(context.module);
    context.variable_map["__state"] = state_reg;
    
    // Initialize state to 0 (start)
    std::vector<Operand> init_ops;
    Operand zero_op = context.generator.createImmediateOperand(0);
    init_ops.push_back(zero_op);
    
    Instruction init_instr = context.generator.createInstruction(OP_CONST, state_reg, init_ops);
    context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, init_instr);
    
    // Create coroutine handle
    Register coro_reg = context.generator.allocateRegister(context.module);
    context.variable_map["__coro"] = coro_reg;
    
    // Create coroutine
    std::vector<Operand> coro_ops;
    Operand func_op = context.generator.createFunctionOperand(context.current_func_id);
    coro_ops.push_back(func_op);
    
    Instruction coro_instr = context.generator.createInstruction(OP_CORO_CREATE, coro_reg, coro_ops);
    context.generator.addInstruction(context.module, context.current_func_id, context.current_bb, coro_instr);
    
    // Generate state machine for async function body
    int state_id = 0;
    BasicBlockId current_state_bb = context.current_bb;
    
    for (const auto& stmt : func_decl->body) {
        // Create state block for each significant point in the async function
        BasicBlockId state_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
        
        // Generate state transition
        if (state_id > 0) {
            // Jump to current state based on state variable
            std::vector<Operand> branch_ops;
            Operand state_op = context.generator.createRegisterOperand(state_reg);
            Operand target_op = context.generator.createBasicBlockOperand(state_bb);
            Operand next_op = context.generator.createBasicBlockOperand(state_bb + 1);
            branch_ops.push_back(state_op);
            branch_ops.push_back(target_op);
            branch_ops.push_back(next_op);
            
            Instruction branch_instr = context.generator.createInstruction(OP_BRANCH, -1, branch_ops);
            context.generator.addInstruction(context.module, context.current_func_id, current_state_bb, branch_instr);
        }
        
        // Set current block to state block
        context.current_bb = state_bb;
        
        // Handle yield statements specially
        if (auto yield_stmt = std::dynamic_pointer_cast<tang::YieldStmt>(stmt)) {
            // Yield point - save state and suspend
            std::vector<Operand> yield_ops;
            if (yield_stmt->expr) {
                Register yield_reg = generateExprIR(context, yield_stmt->expr);
                Operand value_op = context.generator.createRegisterOperand(yield_reg);
                yield_ops.push_back(value_op);
            }
            
            // Update state for next resumption
            std::vector<Operand> state_ops;
            Operand next_state_op = context.generator.createImmediateOperand(state_id + 1);
            state_ops.push_back(next_state_op);
            
            Instruction state_instr = context.generator.createInstruction(OP_CONST, state_reg, state_ops);
            context.generator.addInstruction(context.module, context.current_func_id, state_bb, state_instr);
            
            // Yield coroutine
            Instruction yield_instr = context.generator.createInstruction(OP_CORO_YIELD, -1, yield_ops);
            context.generator.addInstruction(context.module, context.current_func_id, state_bb, yield_instr);
            
        } else {
            // Normal statement processing
            generateStmtIR(context, stmt);
        }
        
        current_state_bb = state_bb;
        state_id++;
    }
    
    // Final state - return from async function
    BasicBlockId final_bb = context.generator.createBasicBlock(context.module, context.current_func_id);
    
    // Jump to final state
    std::vector<Operand> jmp_ops;
    Operand final_op = context.generator.createBasicBlockOperand(final_bb);
    jmp_ops.push_back(final_op);
    
    Instruction jmp_instr = context.generator.createInstruction(OP_JMP, -1, jmp_ops);
    context.generator.addInstruction(context.module, context.current_func_id, current_state_bb, jmp_instr);
    
    // Generate final state
    context.current_bb = final_bb;
    
    // Destroy coroutine and return
    std::vector<Operand> destroy_ops;
    Operand coro_handle_op = context.generator.createRegisterOperand(coro_reg);
    destroy_ops.push_back(coro_handle_op);
    
    Instruction destroy_instr = context.generator.createInstruction(OP_CORO_DESTROY, -1, destroy_ops);
    context.generator.addInstruction(context.module, context.current_func_id, final_bb, destroy_instr);
    
    // Return from async function
    Instruction ret_instr = context.generator.createInstruction(OP_RET, -1, {});
    context.generator.addInstruction(context.module, context.current_func_id, final_bb, ret_instr);
}

} // namespace ir
} // namespace tang
