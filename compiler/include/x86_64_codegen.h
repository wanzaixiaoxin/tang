#pragma once

#include "ir.h"
#include <string>
#include <vector>
#include <iostream>

namespace tang {

namespace ir {

class X86_64CodeGenerator : public CodeGenerator {
public:
    X86_64CodeGenerator();
    
    // Generate machine code
    void generateCode(const Module& module, const std::string& output_file) override;
    
private:
    // Register allocation map
    struct RegisterMap {
        Register virtual_reg;
        std::string physical_reg;
    };
    
    std::vector<RegisterMap> register_allocation;
    std::string current_function;
    
    // Generate assembly file
    void generateAssembly(const Module& module, const std::string& output_file);
    
    // Generate function
    void generateFunction(const Function& func, std::ostream& out);
    
    // Generate basic block
    void generateBasicBlock(const BasicBlock& bb, std::ostream& out);
    
    // Generate instruction
    void generateInstruction(const Instruction& instr, std::ostream& out);
    
    // Generate operand
    std::string generateOperand(const Operand& op);
    
    // Register allocation
    std::string allocateRegister(Register virt_reg);
    
    // Helper function
    std::string getPhysicalRegister(Register virt_reg);
    
    // Generate data section
    void generateDataSection(const Module& module, std::ostream& out);
    
    // Generate text section start
    void generateTextSectionStart(std::ostream& out);
    
    // Generate text section end
    void generateTextSectionEnd(std::ostream& out);
};

} // namespace ir

} // namespace tang
