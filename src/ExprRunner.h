
#pragma once

#include "ast.h"

namespace lwnn {


    namespace ExprRunner {

        void init();

        float runFloatExpr(std::unique_ptr<const Expr> expr);
        int runInt32Expr(std::unique_ptr<const Expr> expr);

        std::string compile(std::unique_ptr<const Expr> expr);
        std::string compile(std::unique_ptr<const Module> expr);
    }
}

