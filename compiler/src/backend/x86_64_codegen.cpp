#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "../../include/x86_64_codegen.h"

namespace tang {

namespace ir {

X86_64CodeGenerator::X86_64CodeGenerator() {
    // Initialize register allocation table
    // Simple register allocation: map virtual registers to physical registers
    // Using System V AMD64 calling convention
    
    // General purpose registers list
    std::vector<std::string> physical_registers = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    
    // Initialize register mappings
    for (size_t i = 0; i < physical_registers.size(); ++i) {
        RegisterMap reg_map;
        reg_map.virtual_reg = i;
        reg_map.physical_reg = physical_registers[i];
        register_allocation.push_back(reg_map);
    }
}

void X86_64CodeGenerator::generateCode(const Module& module, const std::string& output_file) {
    // Generate assembly file
    generateAssembly(module, output_file);
    
    // Here can add code to call assembler and linker
    // Simplified processing, only generate assembly file
}

void X86_64CodeGenerator::generateAssembly(const Module& module, const std::string& output_file) {
    std::ofstream out(output_file);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }
    
    // Generate assembly file header
    out << ".section .data" << std::endl;
    out << ".align 8" << std::endl;
    
    // Generate data section
    generateDataSection(module, out);
    
    // Generate text section start
    generateTextSectionStart(out);
    
    // Generate all functions
    for (int i = 0; i < module.functions.size(); i++) {
        const Function& func = module.functions[i];
        generateFunction(func, out);
    }
    
    // Generate text section end
    generateTextSectionEnd(out);
    
    out.close();
}

void X86_64CodeGenerator::generateDataSection(const Module& module, std::ostream& out) {
    // Simplified processing, no data section for now
    out << "\n";
}

void X86_64CodeGenerator::generateTextSectionStart(std::ostream& out) {
    out << ".section .text" << std::endl;
    out << ".global main" << std::endl;
    out << ".align 4" << std::endl;
}

void X86_64CodeGenerator::generateTextSectionEnd(std::ostream& out) {
    // Simplified processing, no additional content for now
    out << "\n";
}

void X86_64CodeGenerator::generateFunction(const Function& func, std::ostream& out) {
    current_function = func.name;
    
    // Generate function label
    out << func.name << ":" << std::endl;
    
    // Generate function prologue
    out << "    pushq %rbp" << std::endl;
    out << "    movq %rsp, %rbp" << std::endl;
    
    // Allocate stack space for local variables
    // Simplified processing, no allocation for now
    
    // Generate all basic blocks
    for (int i = 0; i < func.basic_blocks.size(); i++) {
        const BasicBlock& bb = func.basic_blocks[i];
        generateBasicBlock(bb, out);
    }
    
    // Generate function epilogue
    out << "    popq %rbp" << std::endl;
    out << "    ret" << std::endl;
    out << "\n";
}

void X86_64CodeGenerator::generateBasicBlock(const BasicBlock& bb, std::ostream& out) {
    // Generate basic block label
    out << "bb" << bb.id << ":" << std::endl;
    
    // Generate all instructions
    for (int i = 0; i < bb.instructions.size(); i++) {
        const Instruction& instr = bb.instructions[i];
        generateInstruction(instr, out);
    }
}

void X86_64CodeGenerator::generateInstruction(const Instruction& instr, std::ostream& out) {
    out << "    ";
    
    switch (instr.op_code) {
        case tang::ir::OP_ADD:
            if (instr.operands.size() == 2) {
                std::string dst = generateOperand(instr.operands[0]);
                std::string src = generateOperand(instr.operands[1]);
                out << "addq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_SUB:
            if (instr.operands.size() == 2) {
                std::string dst = generateOperand(instr.operands[0]);
                std::string src = generateOperand(instr.operands[1]);
                out << "subq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_MUL:
            if (instr.operands.size() == 2) {
                std::string dst = generateOperand(instr.operands[0]);
                std::string src = generateOperand(instr.operands[1]);
                out << "imulq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_DIV:
            if (instr.operands.size() == 2) {
                // x86_64 division uses fixed registers
                std::string src = generateOperand(instr.operands[1]);
                out << "movq " << src << ", %rax" << std::endl;
                out << "cqto" << std::endl;
                out << "idivq " << generateOperand(instr.operands[0]) << std::endl;
                out << "movq %rax, " << generateOperand(instr.operands[0]) << std::endl;
            }
            break;
        case tang::ir::OP_MOD:
            if (instr.operands.size() == 2) {
                // x86_64 mod uses fixed registers
                std::string src = generateOperand(instr.operands[1]);
                out << "movq " << src << ", %rax" << std::endl;
                out << "cqto" << std::endl;
                out << "idivq " << generateOperand(instr.operands[0]) << std::endl;
                out << "movq %rdx, " << generateOperand(instr.operands[0]) << std::endl;
            }
            break;
        case tang::ir::OP_EQ:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "sete %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_NEQ:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "setne %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_LT:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "setl %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_LTE:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "setle %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_GT:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "setg %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_GTE:
            if (instr.operands.size() == 2) {
                std::string left = generateOperand(instr.operands[0]);
                std::string right = generateOperand(instr.operands[1]);
                out << "cmpq " << right << ", " << left << std::endl;
                out << "setge %al" << std::endl;
                out << "movzbl %al, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_AND:
            if (instr.operands.size() == 2) {
                std::string dst = generateOperand(instr.operands[0]);
                std::string src = generateOperand(instr.operands[1]);
                out << "andq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_OR:
            if (instr.operands.size() == 2) {
                std::string dst = generateOperand(instr.operands[0]);
                std::string src = generateOperand(instr.operands[1]);
                out << "orq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_NOT:
            if (instr.operands.size() == 1) {
                std::string reg = generateOperand(instr.operands[0]);
                out << "notq " << reg << std::endl;
            }
            break;
        case tang::ir::OP_LOAD:
            if (instr.operands.size() == 1) {
                std::string mem = generateOperand(instr.operands[0]);
                out << "movq " << mem << ", " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_STORE:
            if (instr.operands.size() == 2) {
                std::string src = generateOperand(instr.operands[0]);
                std::string dst = generateOperand(instr.operands[1]);
                out << "movq " << src << ", " << dst << std::endl;
            }
            break;
        case tang::ir::OP_ALLOC:
            // Simplified processing, use stack allocation
            if (instr.operands.size() == 1) {
                std::string size = generateOperand(instr.operands[0]);
                out << "subq " << size << ", %rsp" << std::endl;
                out << "movq %rsp, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_FREE:
            // Simplified processing, no memory free
            break;
        case tang::ir::OP_JMP:
            if (instr.operands.size() == 1) {
                std::string bb = generateOperand(instr.operands[0]);
                out << "jmp " << bb << std::endl;
            }
            break;
        case tang::ir::OP_BRANCH:
            if (instr.operands.size() == 3) {
                std::string cond = generateOperand(instr.operands[0]);
                std::string true_bb = generateOperand(instr.operands[1]);
                std::string false_bb = generateOperand(instr.operands[2]);
                
                // Simplified processing, assume condition is comparison result
                out << "cmpq $0, " << cond << std::endl;
                out << "jne " << true_bb << std::endl;
                out << "jmp " << false_bb << std::endl;
            }
            break;
        case tang::ir::OP_CALL:
            if (instr.operands.size() == 1) {
                std::string func = generateOperand(instr.operands[0]);
                out << "call " << func << std::endl;
                if (instr.dst != -1) {
                    out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
                }
            }
            break;
        case tang::ir::OP_RET:
            if (instr.operands.size() == 1) {
                std::string ret_val = generateOperand(instr.operands[0]);
                out << "movq " << ret_val << ", %rax" << std::endl;
            }
            out << "popq %rbp" << std::endl;
            out << "ret" << std::endl;
            break;
        case tang::ir::OP_CONST:
            if (instr.operands.size() == 1) {
                std::string val = generateOperand(instr.operands[0]);
                out << "movq $" << val << ", " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_CORO_CREATE:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 2) {
                std::string func = generateOperand(instr.operands[0]);
                std::string stack_size = generateOperand(instr.operands[1]);
                out << "movq $" << stack_size << ", %rdi" << std::endl;
                out << "movq $" << func << ", %rsi" << std::endl;
                out << "call coro_create" << std::endl;
                out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
            }
            break;
        case tang::ir::OP_CORO_YIELD:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 1) {
                std::string coro = generateOperand(instr.operands[0]);
                out << "movq " << coro << ", %rdi" << std::endl;
                out << "call coro_yield" << std::endl;
            }
            break;
        case tang::ir::OP_CORO_RESUME:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 1) {
                std::string coro = generateOperand(instr.operands[0]);
                out << "movq " << coro << ", %rdi" << std::endl;
                out << "call coro_resume" << std::endl;
                if (instr.dst != -1) {
                    out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
                }
            }
            break;
        case tang::ir::OP_CORO_DESTROY:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 1) {
                std::string coro = generateOperand(instr.operands[0]);
                out << "movq " << coro << ", %rdi" << std::endl;
                out << "call coro_destroy" << std::endl;
            }
            break;
        case tang::ir::OP_ASYNC_READ:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 3) {
                std::string fd = generateOperand(instr.operands[0]);
                std::string buf = generateOperand(instr.operands[1]);
                std::string size = generateOperand(instr.operands[2]);
                out << "movq " << fd << ", %rdi" << std::endl;
                out << "movq " << buf << ", %rsi" << std::endl;
                out << "movq " << size << ", %rdx" << std::endl;
                out << "call async_read" << std::endl;
                if (instr.dst != -1) {
                    out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
                }
            }
            break;
        case tang::ir::OP_ASYNC_WRITE:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 3) {
                std::string fd = generateOperand(instr.operands[0]);
                std::string buf = generateOperand(instr.operands[1]);
                std::string size = generateOperand(instr.operands[2]);
                out << "movq " << fd << ", %rdi" << std::endl;
                out << "movq " << buf << ", %rsi" << std::endl;
                out << "movq " << size << ", %rdx" << std::endl;
                out << "call async_write" << std::endl;
                if (instr.dst != -1) {
                    out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
                }
            }
            break;
        case tang::ir::OP_ASYNC_WAIT:
            // Simplified processing, call runtime function
            if (instr.operands.size() >= 1) {
                std::string async_op = generateOperand(instr.operands[0]);
                out << "movq " << async_op << ", %rdi" << std::endl;
                out << "call async_wait" << std::endl;
                if (instr.dst != -1) {
                    out << "movq %rax, " << allocateRegister(instr.dst) << std::endl;
                }
            }
            break;
        default:
            out << "# Unknown opcode: " << instr.op_code << std::endl;
            break;
    }
}

std::string X86_64CodeGenerator::generateOperand(const Operand& op) {
    switch (op.type) {
        case tang::ir::REGISTER:
            return getPhysicalRegister(op.value.reg);
        case tang::ir::IMMEDIATE:
            return std::to_string(op.value.imm);
        case tang::ir::BASIC_BLOCK:
            return "bb" + std::to_string(op.value.bb);
        case tang::ir::FUNCTION:
            // Simplified processing, return function ID directly
            return "func" + std::to_string(op.value.func);
        case tang::ir::MEMORY:
            {
                std::string base = getPhysicalRegister(op.value.mem.base);
                int32_t offset = op.value.mem.offset;
                if (offset == 0) {
                    return "(" + base + ")";
                } else if (offset > 0) {
                    return std::to_string(offset) + "(" + base + ")";
                } else {
                    return "-" + std::to_string(-offset) + "(" + base + ")";
                }
            }
        default:
            return "# Unknown operand type";
    }
}

std::string X86_64CodeGenerator::allocateRegister(Register virt_reg) {
    // Simple register allocation: if virtual register is mapped, return mapped physical register
    // Otherwise, use new physical register
    
    for (const auto& reg_map : register_allocation) {
        if (reg_map.virtual_reg == virt_reg) {
            return reg_map.physical_reg;
        }
    }
    
    // If not found, add new mapping
    // Simplified processing, assume enough physical registers
    RegisterMap new_reg_map;
    new_reg_map.virtual_reg = virt_reg;
    new_reg_map.physical_reg = "r" + std::to_string(virt_reg);
    register_allocation.push_back(new_reg_map);
    
    return new_reg_map.physical_reg;
}

std::string X86_64CodeGenerator::getPhysicalRegister(Register virt_reg) {
    for (const auto& reg_map : register_allocation) {
        if (reg_map.virtual_reg == virt_reg) {
            return reg_map.physical_reg;
        }
    }
    
    // If not found, return default register
    return allocateRegister(virt_reg);
}

} // namespace ir

} // namespace tang
