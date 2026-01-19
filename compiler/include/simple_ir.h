#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tang {
namespace ir {

// 简化的IR模块定义
struct Module {
    std::vector<int> functions; // 简化版本
};

// 简化的代码生成器基类
class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;
    virtual void generateCode(const Module& module, const std::string& output_file) = 0;
};

// 简化的IR生成函数
std::shared_ptr<Module> generateIR(const std::shared_ptr<Module>& ast);

// 简化的代码生成器工厂
std::unique_ptr<CodeGenerator> createX86_64CodeGenerator();

// 简化的IR打印函数
void printIR(const Module& module);

} // namespace ir
} // namespace tang