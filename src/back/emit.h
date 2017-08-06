
#include "front/ast.h"
#include "CompileContext.h"

#pragma once

namespace lwnn {
    namespace back {
        const int ALIGNMENT = 4;

        llvm::Value *emitExpr(lwnn::ast::ExprStmt *exprStmt, CompileContext &);

    }
}