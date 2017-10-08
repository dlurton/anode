#pragma once

#include "CompileContext.h"

namespace anode { namespace back {

class CompileAstVisitor : public front::ast::AstVisitor {
    CompileContext &cc_;
protected:
    CompileContext &cc() { return cc_; }
    explicit CompileAstVisitor(CompileContext &cc) : cc_(cc) { }
};

}}