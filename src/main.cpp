#include <iostream>
#include "codegen.h"
#include "node.h"
 
using namespace std;
 
extern int yyparse();
extern NBlock* programBlock;
 
int main(int argc, char **argv)
{
    yyparse();
    std::cout << programBlock << endl;
    InitializeNativeTarget(); 
    CodeGenContext context;
    context.generateCode(*programBlock);
    context.runCode();
 
    return 0;
}
