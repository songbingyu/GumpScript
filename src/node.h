/*
*  CopyRight  2014 , bingyu.song   All Right Reserved
*  I believe  Spring brother
*/

#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>

class CodeGenContext;
class NStatement;
class NExpression;
class NVariableDeclaration;

typedef std::vector<NStatement*>  StatementList;
typedef	std::vector<NExpression*> ExpressionList;
typedef std::vector<NVariableDeclaration*> VariableList;

class Node {
public:
	virtual ~Node(){}
	virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};

class NExpression : public Node {

};

class NStatement : public Node {

};

class NInteger : public NExpression {
public:
	NInteger(long long value) : value_(value) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	long long value_;
};

class NDouble : public NExpression {
public:
	NDouble(double value) : value_(value) {}
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	double value_;
};

class NIdentifier : public NExpression {
public:
	NIdentifier(std::string name) : name_(name) {}
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	std::string name_;
};

class NMethodCall : public NExpression {
public:
	NMethodCall(const NIdentifier& id, ExpressionList& list) : id_(id), argList_(list) {}
	NMethodCall(const NIdentifier& id) : id_(id) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	const NIdentifier& id_;
	ExpressionList argList_;
}; 

class NBinaryOperator : public NExpression {
public:
	NBinaryOperator(NExpression& lhs, int op, NExpression& rhs) : lhs_(lhs), 
											op_(op), rhs_(rhs) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	NExpression& lhs_;
	int op_;
	NExpression& rhs_;
};

class NAssignment : public NExpression {
public:
	NAssignment(NIdentifier& lhs, NExpression& rhs) : lhs_(lhs), rhs_(rhs) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	NIdentifier& lhs_;
	NExpression& rhs_;
};

class NBlock : public NExpression {
public:
	NBlock() {}
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	StatementList statements_;
};

class NExpressionStatement : public NStatement {
public:
	NExpressionStatement(NExpression& expr) : expr_(expr) {}
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	NExpression& expr_;
};

class NReturnStatement : public NStatement {
public:
    NReturnStatement(NExpression& expr) : 
                            expr_(expr) { }

	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
    NExpression& expr_;
};

class NVariableDeclaration : public NStatement {
public:
	NVariableDeclaration(const NIdentifier& type, NIdentifier& id) :
												type_(type), id_(id) {}
	NVariableDeclaration(const NIdentifier& type, NIdentifier& id, NExpression* expr) :
		type_(type), id_(id), assignExpr_(expr) {} 
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	const NIdentifier& type_;
	NIdentifier& id_;
	NExpression* assignExpr_;
};

class NExternDeclaration : public NStatement {
public:
    NExternDeclaration(const NIdentifier& type, const NIdentifier& id, VariableList& args) :
                        type_(type), id_(id), args_(args) { } 
    virtual llvm::Value* codeGen(CodeGenContext& context);

public:
    const NIdentifier& type_;
    const NIdentifier& id_;
    VariableList args_;
};

class NFunctionDeclaration : public NStatement {
public:
	NFunctionDeclaration(const NIdentifier& type, const NIdentifier& id,
		const VariableList& args, NBlock& block) : type_(type), id_(id),
		args_(args), block_(block) {} 
	virtual llvm::Value* codeGen(CodeGenContext& context);

public:
	const NIdentifier& type_;
	const NIdentifier& id_;
	VariableList args_;
	NBlock& block_;
};

