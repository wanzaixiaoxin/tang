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

// AST to IR conversion function
Module generateIR(const std::shared_ptr<tang::Module>& ast_module) {
    IRGenerator generator;
    Module ir_module = generator.createModule();
    
    // Traverse AST module, generate IR
    for (size_t i = 0; i < ast_module->functions.size(); i++) {
        const std::shared_ptr<FunctionDecl>& func_decl = ast_module->functions[i];
        // Determine function type
        FunctionType func_type = func_decl->is_sync ? SYNC : ASYNC;
        
        // Create IR function
        FunctionId func_id = generator.createFunction(ir_module, func_decl->name, func_type);
        
        // Create entry and exit basic blocks
        BasicBlockId entry_bb = generator.createBasicBlock(ir_module, func_id);
        BasicBlockId exit_bb = generator.createBasicBlock(ir_module, func_id);
        generator.setEntryBlock(ir_module, func_id, entry_bb);
        generator.setExitBlock(ir_module, func_id, exit_bb);
        
        // Allocate parameter registers
        for (size_t j = 0; j < func_decl->params.size(); j++) {
            const auto& param = func_decl->params[j];
            const std::string& param_name = param.first;
            const std::shared_ptr<Type>& param_type = param.second;
            
            Register reg = generator.allocateRegister(ir_module);
            generator.addParameter(ir_module, func_id, reg);
            
            // Generate parameter load instruction (simplified processing)
            std::vector<Operand> load_ops;
            Operand mem_op;
            mem_op.type = MEMORY;
            mem_op.value.mem.base = reg;
            mem_op.value.mem.offset = 0;
            load_ops.push_back(mem_op);
            
            Instruction instr;
            instr.op_code = OP_LOAD;
            instr.dst = reg;
            instr.operands = load_ops;
            
            generator.addInstruction(ir_module, func_id, entry_bb, instr);
        }
        
        // Generate function body instructions (simplified processing, actual implementation needs to traverse AST statements)
        // Only generate a simple return instruction here
        Instruction ret_instr;
        ret_instr.op_code = OP_RET;
        ret_instr.dst = -1;
        ret_instr.operands = std::vector<Operand>();
        generator.addInstruction(ir_module, func_id, exit_bb, ret_instr);
        
        // Add jump from entry block to exit block
        std::vector<Operand> jmp_ops;
        Operand bb_op;
        bb_op.type = BASIC_BLOCK;
        bb_op.value.bb = exit_bb;
        jmp_ops.push_back(bb_op);
        
        Instruction jmp_instr;
        jmp_instr.op_code = OP_JMP;
        jmp_instr.dst = -1;
        jmp_instr.operands = jmp_ops;
        
        generator.addInstruction(ir_module, func_id, entry_bb, jmp_instr);
    }
    
    return ir_module;
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

} // namespace ir
} // namespace tang
