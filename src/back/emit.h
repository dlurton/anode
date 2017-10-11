
#include "front/ast.h"
#include "CompileContext.h"

#pragma once

namespace anode {
    namespace back {
        const int ALIGNMENT = 8;

        llvm::Value *emitExpr(anode::front::ast::ExprStmt *exprStmt, CompileContext &);
        void emitFuncDefs(anode::front::ast::Module *module, CompileContext &cc);

    }
}