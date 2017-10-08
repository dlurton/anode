

#include "CompileAstVisitor.h"
#include "front/ast.h"
#include "emit.h"

namespace anode { namespace back {
using namespace anode::front;

class DeclareFuncsAstVisitor : public CompileAstVisitor {
private:

    void declareFunction(front::scope::FunctionSymbol *functionSymbol) {

        //Map all Anode argument types to LLVM types.
        gc_vector<type::Type*> anodeParamTypes = functionSymbol->functionType()->parameterTypes();
        std::vector<llvm::Type*> llvmParamTypes;

        //Create "this" argument, if needed.
        if(functionSymbol->storageKind() == scope::StorageKind::Instance) {
            llvmParamTypes.reserve(anodeParamTypes.size() + 1);
            llvm::Type *thisType = cc().typeMap().toLlvmType(functionSymbol->thisSymbol()->type());
            llvmParamTypes.push_back(thisType);
        } else {
            llvmParamTypes.reserve(anodeParamTypes.size());
        }

        //Create other arguments.
        for(auto anodeType : anodeParamTypes) {
            llvm::Type *llvmType = cc().typeMap().toLlvmType(anodeType);
            llvmParamTypes.push_back(llvmType);
        }

        llvm::Type *returnLlvmType = cc().typeMap().toLlvmType(functionSymbol->functionType()->returnType());

        //Create the LLVM FunctionType
        llvm::FunctionType *functionType = llvm::FunctionType::get(returnLlvmType, llvmParamTypes, /*isVarArg*/ false);

        auto * llvmFunc = llvm::cast<llvm::Function>(
            cc().llvmModule().getOrInsertFunction(functionSymbol->fullyQualifiedName(), functionType));

        llvmFunc->setCallingConv(llvm::CallingConv::C);

        cc().mapSymbolToValue(functionSymbol, llvmFunc);
    }

public:
    explicit DeclareFuncsAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) { }

    void visitingModule(ast::Module *) override {

        //All external functions must be added to the current llvm module.
        //TODO:  emit only those functions which are actually referenced.
        gc_vector<scope::FunctionSymbol*> functions = cc().world().globalScope()->functions();
        for(scope::FunctionSymbol *functionSymbol : functions) {
            //Skip functions that are not defined externally - we do that in visitingFuncDefStmt, below...
            if(functionSymbol->isExternal())
                declareFunction(functionSymbol);
        }
    }

    void visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
        scope::FunctionSymbol *functionSymbol = funcDef->symbol();

        declareFunction(functionSymbol);
    }
};

class DefineFuncsAstVisitor : public CompileAstVisitor {

    void emitCopyParameterToLocal(llvm::Argument &argument, scope::Symbol *argumentSymbol) {
        argument.setName(argumentSymbol->name());
        llvm::Type *localParamType = cc().typeMap().toLlvmType(argumentSymbol->type());

        llvm::AllocaInst *localParamValue = cc().irBuilder().CreateAlloca(localParamType);
        localParamValue->setName("local_" + argumentSymbol->name());
        cc().irBuilder().CreateStore(&argument, localParamValue);

        cc().mapSymbolToValue(argumentSymbol, localParamValue);
    }

public:
    explicit DefineFuncsAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}

    void visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
        //Save the state of the IRBuilder so it can be restored later
        auto oldBasicBlock = cc().irBuilder().GetInsertBlock();
        auto oldInsertPoint = cc().irBuilder().GetInsertPoint();

        llvm::Value *functionPtr = cc().getMappedValue(funcDef->symbol());

        auto *llvmFunc = llvm::cast<llvm::Function>(functionPtr);
        auto startBlock = llvm::BasicBlock::Create(cc().llvmContext(), "begin", llvmFunc);

        cc().irBuilder().SetInsertPoint(startBlock);

        // Copy parameters to local variables where needed and map them to their symbols.  If this seems weird, that's because it is...
        // However, this appears to be exactly what clang does with optimization disabled!  I compiled the following test program here:
        // http://ellcc.org/demo/index.cgi to discover this...
        //    int foo(int a, int b) { return a + b + 1; }
        //    int main() { foo(1, 2); }
        // The reason Anode needs to do this is because LLVM weirdly treats its arguments as values while all other
        // variables are pointers.  This allows the anode front-end to treat *all* variables as pointers and we don't need to include
        // special logic to determine if the values referenced by pointers need to be loaded.
        // I don't think this is very much of a performance hit because LLVM should optimize this out...  After all, clang does it.

        auto llvmParamItr = llvmFunc->arg_begin();

        if(funcDef->symbol()->storageKind() == scope::StorageKind::Instance) {
            emitCopyParameterToLocal((*llvmParamItr), funcDef->symbol()->thisSymbol());
            ++llvmParamItr;
        }
        auto funcParameters = funcDef->parameters();
        int argCount = 0;
        for(; llvmParamItr != llvmFunc->arg_end(); ++llvmParamItr) {
            ast::ParameterDef *parameterDef = funcParameters[argCount++];
            llvm::Argument &argument = (*llvmParamItr);
            emitCopyParameterToLocal(argument, parameterDef->symbol());
        }

        cc().pushFuncDefStmt(funcDef);
        llvm::Value *returnValue = emitExpr(funcDef->body(), cc());
        cc().popFuncDefStmt();

        //If return type is not void...
        if(!funcDef->returnType()->isVoid()) {
            cc().irBuilder().CreateRet(returnValue);
        } else {
            cc().irBuilder().CreateRetVoid();
        }

        //Restore the state of the IR builder.
        cc().irBuilder().SetInsertPoint(oldBasicBlock, oldInsertPoint);
    }
};


void emitFuncDefs(front::ast::Module *module, CompileContext &cc) {
    DeclareFuncsAstVisitor declaringVisitor{cc};
    module->accept(&declaringVisitor);

    DefineFuncsAstVisitor definingVisitor{cc};
    module->accept(&definingVisitor);
}

}}
