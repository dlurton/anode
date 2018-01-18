
#pragma once


#include "CompileAstVisitor.h"

namespace anode { namespace back {

class GlobalVariableAstVisitor : public CompileAstVisitor {

    void defineGlobal(front::scope::VariableSymbol &symbol) {
        llvm::Type *llvmType = cc().typeMap().toLlvmType(symbol.type());
        cc().llvmModule().getOrInsertGlobal(symbol.fullyQualifiedName(), llvmType);
        llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(symbol.fullyQualifiedName());
        cc().mapSymbolToValue(symbol, globalVar);
        globalVar->setAlignment(ALIGNMENT);

        if(symbol.type().isClass()) {

            if (symbol.isExternal()) {
                //ExternalWeakLinkage defines a symbol in the current module that can be
                //overridden by a regular ExternalLinkage, it would seem from the LLVM language reference.
                //https://llvm.org/docs/LangRef.html#linkage-types
                //HOWEVER...  in my experience this acts *much* more like the C "extern" keyword,
                //meaning that LLVM seems to expect global variables with ExternalWeakLinkage to be defined
                //in another module, period.  I also note that ExternalWinkLinkage global variables
                //cannot have an initializer without causing assertion failure within the LLVM source.
                globalVar->setLinkage(llvm::GlobalValue::ExternalWeakLinkage);
            } else {
                //Fills the struct instance with zeros
                //globalVar->setInitializer(llvm::ConstantAggregateZero::get(llvmType));

                //ExternalLinkage as far as I can tell is not to be confused with the "extern" C keyword.
                //It seems to mean that the symbol is exposed to other modules, like when the "extern"
                //and "static" keywords are omitted in C.
                globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
            }
        }
    }

public:
    explicit GlobalVariableAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) { }

    void visitingVariableDeclExpr(front::ast::VariableDeclExpr &varDef) override {
        if(varDef.symbol()->storageKind() == front::scope::StorageKind::Global) {
            auto variableSymbol = dynamic_cast<front::scope::VariableSymbol*>(varDef.symbol());
            ASSERT(variableSymbol)
            defineGlobal(*variableSymbol);
        }
    }

    void visitingModule(front::ast::Module &) override {

        //Define all the external global variables now...
        //The reason for doing this here in addition to visitVariableDeclExpr is because symbols defined in other modules (isExternal)
        //do not have VariableDeclExprs in the AST but they do exist as symbols in the global scope.
        //TODO:  emit only those GlobalVariables which are actually referenced...
        gc_ref_vector<front::scope::VariableSymbol> globals = cc().world().globalScope().variables();
        for (front::scope::VariableSymbol &symbol : globals) {
            if(symbol.isExternal()) {
                defineGlobal(symbol);
            }
        }
    }
};


inline void emitGlobals(front::ast::Module *module, CompileContext &cc) {
    GlobalVariableAstVisitor visitor{cc};
    module->acceptVisitor(visitor);
}


}}