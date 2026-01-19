#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include "../../include/ir.h"

namespace tang {
namespace ir {

// x86-64 code generator implementation
class X86_64CodeGenerator : public CodeGenerator {
public:
    X86_64CodeGenerator() {
        // Initialize register mapping
        initRegisterMapping();
    }
    
    void generateCode(const Module& module, const std::string& output_file) override {
        std::stringstream code;
        
        // Generate assembly header
        code << ".text\n";
        code << ".global _start\n";
        code << "\n";
        
        // Generate code for each function
        for (const auto& func : module.functions) {
            generateFunctionCode(code, func);
        }
        
        // Generate entry point
        generateEntryPoint(code, module);
        
        // Write to file
        std::ofstream file(output_file);
        file << code.str();
        file.close();
        
        std::cout << "Generated x86-64 assembly code to: " << output_file << std::endl;
    }
    
private:
    std::unordered_map<Register, std::string> register_map;
    std::unordered_map<OpCode, std::string> opcode_map;
    
    void initRegisterMapping() {
        // Map virtual registers to x86-64 registers
        register_map[0] = "rax";
        register_map[1] = "rbx";
        register_map[2] = "rcx";
        register_map[3] = "rdx";
        register_map[4] = "rsi";
        register_map[5] = "rdi";
        register_map[6] = "r8";
        register_map[7] = "r9";
        register_map[8] = "r10";
        register_map[9] = "r11";
        register_map[10] = "r12";
        register_map[11] = "r13";
        register_map[12] = "r14";
        register_map[13] = "r15";
        
        // Map IR opcodes to x86-64 instructions
        opcode_map[OP_ADD] = "add";
        opcode_map[OP_SUB] = "sub";
        opcode_map[OP_MUL] = "imul";
        opcode_map[OP_DIV] = "idiv";
        opcode_map[OP_MOD] = "idiv"; // mod uses remainder from idiv
        opcode_map[OP_EQ] = "cmp";
        opcode_map[OP_NEQ] = "cmp";
        opcode_map[OP_LT] = "cmp";
        opcode_map[OP_LTE] = "cmp";
        opcode_map[OP_GT] = "cmp";
        opcode_map[OP_GTE] = "cmp";
        opcode_map[OP_AND] = "and";
        opcode_map[OP_OR] = "or";
        opcode_map[OP_NOT] = "not";
        opcode_map[OP_LOAD] = "mov";
        opcode_map[OP_STORE] = "mov";
        opcode_map[OP_JMP] = "jmp";
        opcode_map[OP_BRANCH] = "jmp";
        opcode_map[OP_CALL] = "call";
        opcode_map[OP_RET] = "ret";
        opcode_map[OP_CONST] = "mov";
    }
    
    void generateFunctionCode(std::stringstream& code, const Function& func) {
        code << "# Function: " << func.name << "\n";
        code << func.name << ":\n";
        
        // Function prologue
        code << "    push rbp\n";
        code << "    mov rbp, rsp\n";
        
        // Allocate stack space for local variables
        int stack_size = (func.params.size() + 10) * 8; // Reserve space for params + locals
        code << "    sub rsp, " << stack_size << "\n";
        
        // Save parameters to stack
        for (size_t i = 0; i < func.params.size(); ++i) {
            Register param_reg = func.params[i];
            std::string x86_reg = getX86Register(param_reg);
            int offset = i * 8;
            code << "    mov [rbp - " << (offset + 8) << "], " << x86_reg << "\n";
        }
        
        // Generate code for each basic block
        for (const auto& bb : func.basic_blocks) {
            generateBasicBlockCode(code, bb, func);
        }
        
        // Function epilogue
        code << "." << func.name << "_exit:\n";
        code << "    mov rsp, rbp\n";
        code << "    pop rbp\n";
        code << "    ret\n";
        code << "\n";
    }
    
    void generateBasicBlockCode(std::stringstream& code, const BasicBlock& bb, const Function& func) {
        code << ".L" << bb.id << ":\n";
        
        for (const auto& instr : bb.instructions) {
            generateInstructionCode(code, instr, bb.id, func);
        }
        
        code << "\n";
    }
    
    void generateInstructionCode(std::stringstream& code, const Instruction& instr, BasicBlockId bb_id, const Function& func) {
        std::string dst_reg = instr.dst != -1 ? getX86Register(instr.dst) : "";
        
        switch (instr.op_code) {
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_AND:
            case OP_OR:
                generateBinaryOpCode(code, instr, dst_reg);
                break;
                
            case OP_DIV:
            case OP_MOD:
                generateDivModCode(code, instr, dst_reg);
                break;
                
            case OP_EQ:
            case OP_NEQ:
            case OP_LT:
            case OP_LTE:
            case OP_GT:
            case OP_GTE:
                generateComparisonCode(code, instr, dst_reg, bb_id);
                break;
                
            case OP_NOT:
                generateNotCode(code, instr, dst_reg);
                break;
                
            case OP_LOAD:
                generateLoadCode(code, instr, dst_reg);
                break;
                
            case OP_STORE:
                generateStoreCode(code, instr);
                break;
                
            case OP_JMP:
                generateJumpCode(code, instr);
                break;
                
            case OP_BRANCH:
                generateBranchCode(code, instr, bb_id);
                break;
                
            case OP_CALL:
                generateCallCode(code, instr, dst_reg);
                break;
                
            case OP_RET:
                generateReturnCode(code, instr);
                break;
                
            case OP_CONST:
                generateConstCode(code, instr, dst_reg);
                break;
                
            case OP_ALLOC:
                generateAllocCode(code, instr, dst_reg);
                break;
                
            case OP_FREE:
                generateFreeCode(code, instr);
                break;
                
            default:
                // Generate placeholder for unsupported operations
                code << "    # Unsupported operation: " << static_cast<int>(instr.op_code) << "\n";
                break;
        }
    }
    
    void generateBinaryOpCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 2) {
            std::string left_op = getOperandString(instr.operands[0]);
            std::string right_op = getOperandString(instr.operands[1]);
            
            code << "    " << opcode_map[instr.op_code] << " " << dst_reg << ", " << left_op << "\n";
            code << "    " << opcode_map[instr.op_code] << " " << dst_reg << ", " << right_op << "\n";
        }
    }
    
    void generateDivModCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 2) {
            std::string divisor_op = getOperandString(instr.operands[1]);
            
            // Setup for division: dividend in rax, divisor in specified register
            code << "    mov rax, " << getOperandString(instr.operands[0]) << "\n";
            code << "    mov rcx, " << divisor_op << "\n";
            code << "    cdq\n"; // Sign extend eax into edx
            code << "    idiv rcx\n";
            
            if (instr.op_code == OP_DIV) {
                code << "    mov " << dst_reg << ", rax\n"; // Quotient
            } else { // OP_MOD
                code << "    mov " << dst_reg << ", rdx\n"; // Remainder
            }
        }
    }
    
    void generateComparisonCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg, BasicBlockId bb_id) {
        if (instr.operands.size() >= 2) {
            std::string left_op = getOperandString(instr.operands[0]);
            std::string right_op = getOperandString(instr.operands[1]);
            
            code << "    cmp " << left_op << ", " << right_op << "\n";
            
            // Set result based on comparison
            std::string set_instr;
            switch (instr.op_code) {
                case OP_EQ: set_instr = "sete"; break;
                case OP_NEQ: set_instr = "setne"; break;
                case OP_LT: set_instr = "setl"; break;
                case OP_LTE: set_instr = "setle"; break;
                case OP_GT: set_instr = "setg"; break;
                case OP_GTE: set_instr = "setge"; break;
                default: set_instr = "sete";
            }
            
            code << "    " << set_instr << " al\n";
            code << "    movzx " << dst_reg << ", al\n";
        }
    }
    
    void generateNotCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 1) {
            std::string op = getOperandString(instr.operands[0]);
            code << "    mov " << dst_reg << ", " << op << "\n";
            code << "    not " << dst_reg << "\n";
        }
    }
    
    void generateLoadCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 1) {
            const Operand& mem_op = instr.operands[0];
            if (mem_op.type == MEMORY) {
                std::string base_reg = getX86Register(mem_op.value.mem.base);
                int offset = mem_op.value.mem.offset;
                code << "    mov " << dst_reg << ", [" << base_reg << " + " << offset << "]\n";
            }
        }
    }
    
    void generateStoreCode(std::stringstream& code, const Instruction& instr) {
        if (instr.operands.size() >= 2) {
            const Operand& mem_op = instr.operands[0];
            const Operand& value_op = instr.operands[1];
            
            if (mem_op.type == MEMORY) {
                std::string base_reg = getX86Register(mem_op.value.mem.base);
                int offset = mem_op.value.mem.offset;
                std::string value_str = getOperandString(value_op);
                code << "    mov [" << base_reg << " + " << offset << "], " << value_str << "\n";
            }
        }
    }
    
    void generateJumpCode(std::stringstream& code, const Instruction& instr) {
        if (instr.operands.size() >= 1) {
            const Operand& target_op = instr.operands[0];
            if (target_op.type == BASIC_BLOCK) {
                code << "    jmp .L" << target_op.value.bb << "\n";
            }
        }
    }
    
    void generateBranchCode(std::stringstream& code, const Instruction& instr, BasicBlockId bb_id) {
        if (instr.operands.size() >= 3) {
            const Operand& cond_op = instr.operands[0];
            const Operand& true_op = instr.operands[1];
            const Operand& false_op = instr.operands[2];
            
            std::string cond_str = getOperandString(cond_op);
            
            code << "    cmp " << cond_str << ", 0\n";
            code << "    je .L" << false_op.value.bb << "\n";
            code << "    jmp .L" << true_op.value.bb << "\n";
        }
    }
    
    void generateCallCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 1) {
            // Pass arguments (simplified - first 6 args in registers)
            for (size_t i = 1; i < instr.operands.size() && i <= 6; ++i) {
                std::string arg_reg = getX86Register(static_cast<Register>(i - 1));
                std::string arg_val = getOperandString(instr.operands[i]);
                code << "    mov " << arg_reg << ", " << arg_val << "\n";
            }
            
            // Call function
            const Operand& func_op = instr.operands[0];
            if (func_op.type == FUNCTION) {
                // In real implementation, this would use function name
                code << "    call func" << func_op.value.func << "\n";
            } else {
                // Indirect call
                std::string func_ptr = getOperandString(func_op);
                code << "    call " << func_ptr << "\n";
            }
            
            // Store return value
            if (instr.dst != -1) {
                code << "    mov " << dst_reg << ", rax\n";
            }
        }
    }
    
    void generateReturnCode(std::stringstream& code, const Instruction& instr) {
        if (instr.operands.size() >= 1) {
            std::string ret_val = getOperandString(instr.operands[0]);
            code << "    mov rax, " << ret_val << "\n";
        }
        code << "    jmp .exit\n";
    }
    
    void generateConstCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 1) {
            const Operand& const_op = instr.operands[0];
            if (const_op.type == IMMEDIATE) {
                code << "    mov " << dst_reg << ", " << const_op.value.imm << "\n";
            }
        }
    }
    
    void generateAllocCode(std::stringstream& code, const Instruction& instr, const std::string& dst_reg) {
        if (instr.operands.size() >= 1) {
            std::string size_str = getOperandString(instr.operands[0]);
            
            // Simplified memory allocation using brk system call
            code << "    # Memory allocation for " << size_str << " bytes\n";
            code << "    mov rax, 12\n"; // brk syscall
            code << "    mov rdi, 0\n"; // Get current break
            code << "    syscall\n";
            code << "    mov " << dst_reg << ", rax\n";
            code << "    add rax, " << size_str << "\n";
            code << "    mov rdi, rax\n";
            code << "    mov rax, 12\n";
            code << "    syscall\n";
        }
    }
    
    void generateFreeCode(std::stringstream& code, const Instruction& instr) {
        if (instr.operands.size() >= 1) {
            std::string ptr_str = getOperandString(instr.operands[0]);
            code << "    # Free memory at " << ptr_str << "\n";
            // In real implementation, this would manage a free list
        }
    }
    
    void generateEntryPoint(std::stringstream& code, const Module& module) {
        code << "# Entry point\n";
        code << "_start:\n";
        
        // Call main function if it exists
        bool has_main = false;
        for (const auto& func : module.functions) {
            if (func.name == "main") {
                has_main = true;
                code << "    call main\n";
                break;
            }
        }
        
        if (!has_main && !module.functions.empty()) {
            // Call first function
            code << "    call " << module.functions[0].name << "\n";
        }
        
        // Exit program
        code << "    mov rax, 60\n"; // exit syscall
        code << "    mov rdi, 0\n"; // exit code 0
        code << "    syscall\n";
    }
    
    std::string getX86Register(Register reg) {
        auto it = register_map.find(reg);
        if (it != register_map.end()) {
            return it->second;
        }
        
        // For registers beyond our mapping, use stack locations
        int offset = (reg - register_map.size()) * 8;
        return "[rbp - " + std::to_string(offset + 8) + "]";
    }
    
    std::string getOperandString(const Operand& op) {
        switch (op.type) {
            case REGISTER:
                return getX86Register(op.value.reg);
            case IMMEDIATE:
                return std::to_string(op.value.imm);
            case BASIC_BLOCK:
                return ".L" + std::to_string(op.value.bb);
            case FUNCTION:
                return "func" + std::to_string(op.value.func);
            case MEMORY:
                return "[" + getX86Register(op.value.mem.base) + " + " + 
                       std::to_string(op.value.mem.offset) + "]";
            default:
                return "unknown";
        }
    }
};

// Factory function to create x86-64 code generator
std::unique_ptr<CodeGenerator> createX86_64CodeGenerator() {
    return std::make_unique<X86_64CodeGenerator>();
}

} // namespace ir
} // namespace tang