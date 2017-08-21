
#pragma once

#include "front/ast.h"
#include "CompileAstVisitor.h"
#include "CompileContext.h"

namespace lwnn { namespace back {

/** For each class in the AST, constructs an LLVM type and maps it to its corresponding LWNN type in the CompileContext. */
class CreateStructsAstVisitor : public CompileAstVisitor {
public:
    CreateStructsAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {

    }

    virtual bool visitingClassDefinition(ast::ClassDefinition *cd) {
        gc_vector<scope::Symbol *> symbols = cd->body()->scope()->symbols();
        std::vector<llvm::Type *> memberTypes;
        memberTypes.reserve(symbols.size());

        for (auto s : symbols) {
            memberTypes.push_back(cc().typeMap().toLlvmType(s->type()));
        }

        llvm::StructType *structType = llvm::StructType::create(cc().llvmContext(), memberTypes, cd->name(), false);
        cc().typeMap().mapTypes(cd->classType(), structType);

        //LLVM will not emit an unused struct so create a dummy variable...
        //cc().llvmModule().getOrInsertGlobal(string::format("use_%s", cd->name().c_str()), structType);

        return false;
    }
};



}}