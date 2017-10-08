
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

    void visitingCompleteClassDefinition(front::ast::CompleteClassDefinition *cd) override {
        CompileAstVisitor::visitingCompleteClassDefinition(cd);
        gc_vector<front::scope::VariableSymbol*> symbols = cd->body()->scope()->variables();

        std::vector<llvm::Type *> memberTypes;
        memberTypes.reserve(symbols.size());

        llvm::StructType *structType = llvm::StructType::create(cc().llvmContext(), cd->name());
        cc().typeMap().mapTypes(cd->definedType(), structType->getPointerTo(0));

        for (auto s : symbols) {
            memberTypes.push_back(cc().typeMap().toLlvmType(s->type()));
        }

        structType->setBody(memberTypes);

    }
};



}}