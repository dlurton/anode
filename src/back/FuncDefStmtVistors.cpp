

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

        llvm::Value *returnValue = emitExpr(funcDef->body(), cc());

        //If return type is not void...
        if(!funcDef->returnTypeRef()->type()->isVoid()) {
            cc().irBuilder().CreateRet(returnValue);
        } else {
            cc().irBuilder().CreateRetVoid();
        }

        //Restore the state of the IR builder.
        cc().irBuilder().SetInsertPoint(oldBasicBlock, oldInsertPoint);
        return true;
    }
};

class DeclaringFuncDefDeclStmtVisitor : public CompileAstVisitor {
public:
    DeclaringFuncDefDeclStmtVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}

    bool visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
        llvm::Function* llvmFunc = llvm::cast<llvm::Function>(
            cc().llvmModule().getOrInsertFunction(funcDef->name(), cc().typeMap().toLlvmType(funcDef->returnTypeRef()->type())));

        llvmFunc->setCallingConv(llvm::CallingConv::C);

        cc().mapSymbolToValue(funcDef->symbol(), llvmFunc);
        return true;
    }
};

void emitFuncDefs(lwnn::ast::Module *module, CompileContext &cc) {
    DeclaringFuncDefDeclStmtVisitor declaringVisitor{cc};
    module->accept(&declaringVisitor);

    FuncDefStmtVistors definingVisitor{cc};
    module->accept(&definingVisitor);
}

}}
