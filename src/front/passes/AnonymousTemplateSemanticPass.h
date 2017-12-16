
#pragma once



#include "front/ErrorStream.h"
#include "front/ast.h"
#include "ErrorContextAstVisitor.h"

namespace anode { namespace front  { namespace passes {

class AnonymousTemplatesSemanticPass : public ErrorContextAstVisitor {
public:
    explicit AnonymousTemplatesSemanticPass(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) {}

    void visitingAnonymousTemplateExprStmt(ast::AnonymousTemplateExprStmt &exprStmt) override {
        auto &bodyExprs = exprStmt.body().expressions();
        for(ast::ExprStmt &expr : bodyExprs) {
            if (dynamic_cast<ast::GenericClassDefinition *>(&expr)) {
                continue;
            }
            //TODO:  allow GenericFunctionDefinitions or whatever, once they exist.
            errorStream_.error(
                error::ErrorKind::OnlyClassesAndFunctionsAllowedInAnonymousTemplates,
                expr.sourceSpan(),
                "Only classes and functions allowed in anonymous templates");
        }
    }
};

}}}