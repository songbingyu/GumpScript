#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

void CodeGenContext::generateCode(NBlock& root) {
    std::cout << "Generating code ...\n";
    
    vector<Type*> arg_types;
    FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), 
                                                makeArrayRef(arg_types), false);
    main_func_ = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module_);
    BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", main_func_, 0);
    
    pushBlock(bblock);
    root.codeGen(*this);
    ReturnInst::Create(getGlobalContext(), bblock);
    popBlock(); 
        
    std::cout << "code is generated..\n";
    PassManager pm;
    pm.add(createPrintModulePass(&outs()));
    pm.run(*module_);
}

GenericValue CodeGenContext::runCode() {
    std::cout << "Running code ...\n";
    ExecutionEngine* ee = EngineBuilder(module_).create();
    vector<GenericValue> noargs;
    GenericValue v = ee->runFunction(main_func_, noargs);
    std::cout << "Code was run.\n";
    return v;
}

static Type* typeOf(const NIdentifier& type) {
    if (type.name_.compare("int") == 0) {
        return Type::getInt64Ty(getGlobalContext());
    } else if (type.name_.compare("double") == 0) {
        return Type::getDoubleTy(getGlobalContext());
    }
    return Type::getVoidTy(getGlobalContext());
}

Value* NInteger::codeGen(CodeGenContext& context) {
    std::cout << "Create integer:" << value_ << endl; 
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value_, true);
}

Value* NDouble::codeGen(CodeGenContext& context) {
    std::cout << "Create double:" << value_ << endl; 
    return ConstantInt::get(Type::getDoubleTy(getGlobalContext()), value_);
}

Value* NIdentifier::codeGen(CodeGenContext& context) {
    std::cout << "Create identifier reference: " << name_ << endl;
    if (context.locals().find(name_) == context.locals().end()) {
        std::cerr << "undeclared var" << name_ << endl;
        return NULL;
    }
    return new LoadInst(context.locals()[name_], "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context) {
    Function* function = context.module_->getFunction(id_.name_.c_str());
    if (NULL == function) {
        std::cerr << "no such function" << id_.name_ << endl;
    }
    std::vector<Value*> args;
    ExpressionList::const_iterator it;
    for (it = argList_.begin(); it != argList_.end(); ++it) {
        args.push_back((**it).codeGen(context));
    } 
    CallInst* call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
    std::cout << "Creating method call:" << id_.name_ << endl;
    return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context) {
    std::cout << "Creating binary operation" << op_ << endl;
    Instruction::BinaryOps instr;
    switch (op_) {
        case TPLUS:     instr = Instruction::Add; goto math;
        case TMINUS:    instr = Instruction::Sub; goto math;
        case TMUL:      instr = Instruction::Mul; goto math;
        case TDIV:      instr = Instruction::SDiv; goto math;
    }
    return NULL;

math:
    return BinaryOperator::Create(instr, lhs_.codeGen(context), 
                                 rhs_.codeGen(context), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context) {
    std::cout << "Creating assignment for" << lhs_.name_ << endl;
    if (context.locals().find(lhs_.name_) == context.locals().end()) {
        std::cerr << "undeclared var" << lhs_.name_ << endl;
        return NULL; 
    }
    return new StoreInst(rhs_.codeGen(context), context.locals()[lhs_.name_], false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context) {
    StatementList::const_iterator it;
    Value* last = NULL;
    for (it = statements_.begin(); it != statements_.end(); ++it) {
        std::cout << "Generating code for" << typeid(**it).name() << endl;
        last = (**it).codeGen(context);
    }
    std::cout << "Creating block" << endl;
    return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context) {
    std::cout << "Generating code for " << typeid(expr_).name() << endl;
    return expr_.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context) {
    std::cout << "Generating return code for " << typeid(expr_).name() << endl;
    Value* return_value = expr_.codeGen(context);
    context.setCurrentReturnValue(return_value);
    return return_value;

}

Value* NVariableDeclaration::codeGen(CodeGenContext& context) {
    std::cout << "Createing variable declaration" << type_.name_ << " " << id_.name_ << endl;
    AllocaInst* alloc = new AllocaInst(typeOf(type_), id_.name_.c_str(), context.currentBlock()); 
    context.locals()[id_.name_] = alloc;
    if (assignExpr_ != NULL) {
        NAssignment assn(id_, *assignExpr_);
        assn.codeGen(context);
    }
    return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context) {
    vector<Type*> arg_types;
    VariableList::const_iterator it;
    for (it = args_.begin(); it != args_.end(); it++) {
        arg_types.push_back(typeOf((**it).type_));
    }
    FunctionType* ftype = FunctionType::get(typeOf(type_), makeArrayRef(arg_types), false);
    Function* func = Function::Create(ftype, GlobalValue::ExternalLinkage, id_.name_.c_str(), context.module_);
    return func;    
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context) {
    vector<Type*> arg_types;
    VariableList::const_iterator it;
    for (it = args_.begin(); it != args_.end(); it++) {
        arg_types.push_back(typeOf((**it).type_));
    }
    FunctionType* ftype = FunctionType::get(typeOf(type_), makeArrayRef(arg_types), false);
    Function* func = Function::Create(ftype, GlobalValue::InternalLinkage, id_.name_.c_str(), context.module_);
    BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", func, 0);
    
    context.pushBlock(bblock);
    
    Function::arg_iterator args_values = func->arg_begin();
    Value* arg_value; 
    
    for (it = args_.begin(); it != args_.end(); ++it) {
        (**it).codeGen(context);
        arg_value = args_values++;
        arg_value->setName((*it)->id_.name_.c_str());
        StoreInst* inst = new StoreInst(arg_value, context.locals()[(*it)->id_.name_], false, bblock);
    }
    
    block_.codeGen(context);
    ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(), bblock);
    
    context.popBlock();
    std::cout << "Creating func:" << id_.name_ << endl;
    return func;
}
