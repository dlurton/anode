#pragma once

#include "CompileContext.h"

namespace anode {
    namespace back {
        class CompileAstVisitor : public ast::AstVisitor {
            CompileContext &cc_;
        protected:
            CompileContext &cc() { return cc_; }
        public:
            CompileAstVisitor(CompileContext &cc) : cc_(cc) {

            }
        };

    }
}