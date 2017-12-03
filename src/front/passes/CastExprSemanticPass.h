
#pragma once


#include "front/ErrorStream.h"
#include "front/ast.h"

namespace anode { namespace front  { namespace passes {


class CastExprSemanticPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit CastExprSemanticPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    void visitingCastExpr(ast::CastExpr &expr) override {
        //Note:  we are not excluding implicit casts here...
        //This is a form of "double-checking" the AddImplicitCastsVisitor that we
        //get for free as long as we don't exclude implicit casts.

        type::Type &fromType = expr.valueExpr().type();
        type::Type &toType = expr.type();

        if(fromType.canImplicitCastTo(toType)) return;

        if(expr.castKind() == ast::CastKind::Implicit) {
            ASSERT_FAIL("Implicit cast created for types that can't be implicitly cast.");
        }

        if(!fromType.canExplicitCastTo(toType)) {
            errorStream_.error(
                error::ErrorKind::InvalidExplicitCast,
                expr.sourceSpan(),
                "Cannot cast from '%s' to '%s'",
                fromType.nameForDisplay().c_str(),
                toType.nameForDisplay().c_str());
        }
    }
};


}}}