

#include "CompileAstVisitor.h"
#include "front/ast.h"
#include "emit.h"

namespace lwnn { namespace back {

class FuncDefStmtVistors : public CompileAstVisitor {
public:
    FuncDefStmtVistors(CompileContext &cc) : CompileAstVisitor(cc) {}

    bool visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
        //Save the state of the IRBuilder so it can be restored later
        auto oldBasicBlock = cc().irBuilder().GetInsertBlock();
        auto oldInsertPoint = cc().irBuilder().GetInsertPoint();

        llvm::Value *functionPtr = cc().getMappedValue(funcDef->symbol());

        llvm::Function *llvmFunc = llvm::cast<llvm::Function>(functionPtr);
        auto startBlock = llvm::BasicBlock::Create(cc().llvmContext(), "begin", llvmFunc);

        cc().irBuilder().SetInsertPoint(startBlock);

        // Copy parameters to local variables. If this seems weird, that's because it is...
        // However, this appears to be exactly what clang does with optimization disabled!  I compiled the following test program here:
        // http://ellcc.org/demo/index.cgi to discover this...
        //    int foo(int a, int b) { return a + b + 1; }
        //    int main() { foo(1, 2); }
        // The reason LWNN needs to do it is because LLVM weirdly treats its arguments (that do not need to be loaded) as values instead
        // of pointers that do not need to be loaded.  All of the other variables are pointers whose values need to be loaded.
        // This brings usage of function arguments to be the same as all other variables...
        // I don't think this is very much of a performance hit because LLVM should optimize this out...
        auto funcParameters = funcDef->parameters();
        int argCount = 0;
        for(auto llvmParamItr = llvmFunc->arg_begin(); llvmParamItr != llvmFunc->arg_end(); ++llvmParamItr) {
            ast::ParameterDef *parameterDef = funcParameters[argCount++];
            llvm::Argument &argument = (*llvmParamItr);
            argument.setName(parameterDef->name());

            llvm::AllocaInst *localParamValue = cc().irBuilder().CreateAlloca(argument.getType());
            localParamValue->setName("local_" + parameterDef->name());
            cc().mapSymbolToValue(parameterDef->symbol(), localParamValue);
            cc().irBuilder().CreateStore(&argument, localParamValue);
        }


        llvm::Value *returnValue = emitExpr(funcDef->body(), cc());

        //If return type is not void...
        if(!funcDef->returnType()->isVoid()) {
            cc().irBuilder().CreateRet(returnValue);
        } else {
            cc().irBuilder().CreateRetVoid();
        }

        //Restore the state of the IR builder.
        cc().irBuilder().SetInsertPoint(oldBasicBlock, oldInsertPoint);
        return true;
    }
};
void emitFuncDefs(lwnn::ast::Module *module, CompileContext &cc) {
    FuncDefStmtVistors definingVisitor{cc};
    module->accept(&definingVisitor);
}

}}
