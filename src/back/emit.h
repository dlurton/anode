
#include "front/ast.h"
#include "CompileContext.h"

#pragma once

namespace anode {
    namespace back {
        const int ALIGNMENT = 4;

        llvm::Value *emitExpr(anode::ast::ExprStmt *exprStmt, CompileContext &);
        void emitFuncDefs(anode::ast::Module *module, CompileContext &cc);

    }
}