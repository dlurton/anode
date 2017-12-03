
#pragma once

#include "front/ErrorStream.h"
#include "front/ast.h"

namespace anode { namespace front  { namespace passes {

class ResolveDotExprMemberPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit ResolveDotExprMemberPass(error::ErrorStream &errorStream) : errorStream_{errorStream} { }

    void visitedDotExpr(ast::DotExpr &expr) override {
        if(!expr.lValue().type().isClass()) {
            errorStream_.error(
                error::ErrorKind::LeftOfDotNotClass,
                expr.dotSourceSpan(),
                "Dot operator is not usable with data type of expression on left side of '.' operator: %s",
                expr.lValue().type().nameForDisplay().c_str());
            return;
        }
        auto classType = static_cast<const type::ClassType*>(expr.lValue().type().actualType());
        type::ClassField *field = classType->findField(expr.memberName().text());
        if(!field) {
            errorStream_.error(
                error::ErrorKind::ClassMemberNotFound,
                expr.dotSourceSpan(),
                "Class '%s' does not have a member named '%s'",
                classType->nameForDisplay().c_str(),
                expr.memberName().text().c_str());
            return;
        }
        expr.setField(field);
    }

    void visitedFuncCallExpr(ast::FuncCallExpr &expr) override {
        auto methodRef = dynamic_cast<ast::MethodRefExpr*>(&expr.funcExpr());
        if(methodRef && expr.instanceExpr()) {
            type::Type *instanceType = expr.instanceExpr()->type().actualType();
            type::ClassMethod *method = nullptr;

            if(auto classType = dynamic_cast<type::ClassType*>(instanceType)) {
                method = classType->findMethod(methodRef->name().text());
            }

            if(method) {
                methodRef->setSymbol(method->symbol());
            } else {
                errorStream_.error(
                    error::ErrorKind::MethodNotDefined,
                    methodRef->sourceSpan(),
                    "Type '%s' does not have a method named '%s'.",
                    instanceType->nameForDisplay().c_str(),
                    methodRef->name().text().c_str());
            }
        }
    }
};

}}}