#include <stack>
#include <typeinfo>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
// #include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

class NBlock;

class CodeGenBlock {
public:
    BasicBlock* block_;
    Value*  return_value_;
    std::map<std::string, Value*> locals_;
};


class CodeGenContext {
public:
    CodeGenContext() { module_ = new Module("main", getGlobalContext()); }
    
    void generateCode(NBlock& root);
    GenericValue runCode();
    std::map<std::string, Value*>& locals() { return blocks_.top()->locals_; }
    BasicBlock* currentBlock() { return blocks_.top()->block_; }
    void pushBlock(BasicBlock* block) { 
        blocks_.push(new CodeGenBlock()); 
        blocks_.top()->return_value_ = NULL; 
        blocks_.top()->block_ = block; 
    }
    void popBlock() { CodeGenBlock* top = blocks_.top(); blocks_.pop(); delete top; }
    void setCurrentReturnValue(Value* value) { blocks_.top()->return_value_ = value; }
    Value* getCurrentReturnValue() { return blocks_.top()->return_value_; }

private:
    std::stack<CodeGenBlock*> blocks_;
    Function* main_func_;

public:
    Module* module_;
};
