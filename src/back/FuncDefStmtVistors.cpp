

#include "CompileAstVisitor.h"
#include "front/ast.h"
#include "emit.h"

namespace anode { namespace back {

class DeclareFuncsAstVisitor : public CompileAstVisitor {
private:

    void declareFunction(scope::FunctionSymbol *functionSymbol) {
        //Map all Anode argument types to LLVM types.
        gc_vector<type::Type*> anodeParamTypes = functionSymbol->functionType()->parameterTypes();
        std::vector<llvm::Type*> llvmParamTypes;
        llvmParamTypes.reserve(anodeParamTypes.size());
        for(auto anodeType : anodeParamTypes) {
            llvm::Type *llvmType = cc().typeMap().toLlvmType(anodeType);
//            if(anodeType->isClass()) {
//                llvmType = llvmType->getPointerTo(0);
//            }
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

    bool visitingModule(ast::Module *module) override {

        //All external functions must be added to the current llvm module.
        gc_vector<scope::FunctionSymbol*> functions = module->scope()->functions();
        for(scope::FunctionSymbol *functionSymbol : functions) {
            //Skip functions that are not defined externally - we do that in visitingFuncDefStmt, below...
            if(functionSymbol->isExternal())
                declareFunction(functionSymbol);
        }

        return true;
    }

    bool visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
        scope::FunctionSymbol *functionSymbol = funcDef->symbol();

        declareFunction(functionSymbol);

        return true;
    }
};

class DefineFuncsAstVisitor : public CompileAstVisitor {
public:
    explicit DefineFuncsAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}

    bool visitingFuncDefStmt(ast::FuncDefStmt *funcDef) override {
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
        // I don't think this is very much of a performance hit because LLVM should optimize this out...  I mean, clang does it.
        auto funcParameters = funcDef->parameters();
        int argCount = 0;
        for(auto llvmParamItr = llvmFunc->arg_begin(); llvmParamItr != llvmFunc->arg_end(); ++llvmParamItr) {
            ast::ParameterDef *parameterDef = funcParameters[argCount++];
            llvm::Argument &argument = (*llvmParamItr);
            argument.setName(parameterDef->name());
            llvm::Type *localParamType = cc().typeMap().toLlvmType(parameterDef->type());

            llvm::AllocaInst *localParamValue = cc().irBuilder().CreateAlloca(localParamType);
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


void emitFuncDefs(anode::ast::Module *module, CompileContext &cc) {
    DeclareFuncsAstVisitor declaringVisitor{cc};
    module->accept(&declaringVisitor);

    DefineFuncsAstVisitor definingVisitor{cc};
    module->accept(&definingVisitor);
}

}}
