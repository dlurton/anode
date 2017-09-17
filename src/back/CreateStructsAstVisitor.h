
#pragma once

#include "front/ast.h"
#include "CompileAstVisitor.h"
#include "CompileContext.h"

namespace anode { namespace back {

/** For each class in the AST, constructs an LLVM type and maps it to its corresponding Anode type in the CompileContext. */
class CreateStructsAstVisitor : public CompileAstVisitor {
public:
    explicit CreateStructsAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {

    }

    bool visitingClassDefinition(ast::ClassDefinition *cd) override {
        CompileAstVisitor::visitingClassDefinition(cd);
        gc_vector<scope::VariableSymbol *> symbols = cd->body()->scope()->variables();
        std::vector<llvm::Type *> memberTypes;
        memberTypes.reserve(symbols.size());

        for (auto s : symbols) {
            memberTypes.push_back(cc().typeMap().toLlvmType(s->type()));
        }

        llvm::StructType *structType = llvm::StructType::create(cc().llvmContext(), memberTypes, cd->name(), false);

        cc().typeMap().mapTypes(cd->classType(), structType->getPointerTo(0));

        //LLVM will not emit an unused struct so create a dummy variable...
        //cc().llvmModule().getOrInsertGlobal(string::format("use_%s", cd->name().c_str()), structType);

        return false;
    }
};



}}