
#pragma once

#include "front/ast.h"
#include "ScopeFollowingAstVisitor.h"

#include "front/ErrorStream.h"

namespace anode { namespace front { namespace passes {

class   ErrorContextAstVisitor : public ast::AstVisitor {
protected:
    error::ErrorStream &errorStream_;

    ErrorContextAstVisitor(error::ErrorStream &errorStream) : errorStream_{errorStream} { }
public:

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        errorStream_.pushContextMessage("While inside template expansion at: " + expansion.sourceSpan().toString());
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &) override {
        errorStream_.popContextMessage();
    }
};
}}}
