#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace tang {

namespace ir {

// Register type
typedef int32_t Register;

// Basic block ID
typedef int32_t BasicBlockId;

// Function ID
typedef int32_t FunctionId;

// Operation code
enum OpCode {
    // Basic operations
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    
    // Comparison operations
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    
    // Logical operations
    OP_AND,
    OP_OR,
    OP_NOT,
    
    // Memory operations
    OP_LOAD,
    OP_STORE,
    OP_ALLOC,
    OP_FREE,
    
    // Control flow
    OP_JMP,
    OP_BRANCH,
    OP_CALL,
    OP_RET,
    
    // Constants
    OP_CONST,
    
    // Coroutine operations
    OP_CORO_CREATE,
    OP_CORO_YIELD,
    OP_CORO_RESUME,
    OP_CORO_DESTROY,
    
    // Async IO operations
    OP_ASYNC_READ,
    OP_ASYNC_WRITE,
    OP_ASYNC_WAIT
};

// Operand type
enum OperandType {
    REGISTER,
    IMMEDIATE,
    BASIC_BLOCK,
    FUNCTION,
    MEMORY
};

// Operand
typedef struct {
    OperandType type;
    union {
        Register reg;
        int32_t imm;
        BasicBlockId bb;
        FunctionId func;
        struct {
            Register base;
            int32_t offset;
        } mem;
    } value;
} Operand;

// Instruction
typedef struct {
    OpCode op_code;
    Register dst; // Target register, -1 means no target
    std::vector<Operand> operands;
} Instruction;

// Basic block
class BasicBlock {
public:
    BasicBlockId id;
    std::vector<Instruction> instructions;
};

// Function type
enum FunctionType {
    SYNC,
    ASYNC
};

// Function
typedef struct {
    FunctionId id;
    std::string name;
    FunctionType type;
    std::vector<Register> params;
    std::vector<BasicBlock> basic_blocks;
    BasicBlockId entry_block;
    BasicBlockId exit_block;
} Function;

// Module
typedef struct {
    std::vector<Function> functions;
    int32_t next_func_id;
    int32_t next_bb_id;
    int32_t next_reg_id;
} Module;

// IR generator class
class IRGenerator {
public:
    IRGenerator();
    
    // Create new module
    Module createModule();
    
    // Function operations
    FunctionId createFunction(Module& module, const std::string& name, FunctionType type);
    void addParameter(Module& module, FunctionId func_id, Register reg);
    
    // Basic block operations
    BasicBlockId createBasicBlock(Module& module, FunctionId func_id);
    void setEntryBlock(Module& module, FunctionId func_id, BasicBlockId bb_id);
    void setExitBlock(Module& module, FunctionId func_id, BasicBlockId bb_id);
    
    // Instruction operations
    void addInstruction(Module& module, FunctionId func_id, BasicBlockId bb_id, const Instruction& instr);
    
    // Register allocation
    Register allocateRegister(Module& module);
    
    // Helper functions
    Instruction createInstruction(OpCode op_code, Register dst, const std::vector<Operand>& operands);
    Operand createRegisterOperand(Register reg);
    Operand createImmediateOperand(int32_t imm);
    Operand createBasicBlockOperand(BasicBlockId bb);
    Operand createFunctionOperand(FunctionId func);
    Operand createMemoryOperand(Register base, int32_t offset);
    
private:
    // Internal helper functions
    Function& getFunction(Module& module, FunctionId func_id);
    BasicBlock& getBasicBlock(Module& module, FunctionId func_id, BasicBlockId bb_id);
};

// IR optimizer class
class IROptimizer {
public:
    IROptimizer();
    
    // Optimize module
    void optimize(Module& module);
    
private:
    // Optimize function
    void optimizeFunction(Module& module, Function& func);
    
    // Basic optimization passes
    void constantFolding(Module& module, Function& func);
    void deadCodeElimination(Module& module, Function& func);
    void peepholeOptimization(Module& module, Function& func);
};

// IR to machine code generator (abstract base class)
class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;
    
    // Generate machine code
    virtual void generateCode(const Module& module, const std::string& output_file) = 0;
};

// Function declarations will be in implementation files
// Module generateIR(const std::shared_ptr<tang::Module>& ast);
// void printIR(const Module& module);
// std::unique_ptr<CodeGenerator> createX86_64CodeGenerator();

} // namespace ir

} // namespace tang
