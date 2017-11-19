
#pragma once
#include "front/ast.h"
#include "front/ErrorStream.h"

namespace anode { namespace front { namespace passes {

class AddImplicitCastsPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit AddImplicitCastsPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    // Note:  due to it being a more natural place for it, implicit casts for function call arguments are done in FuncCallSemanticsPass

    void visitedBinaryExpr(ast::BinaryExpr &binaryExpr) override {
        if (binaryExpr.binaryExprKind() == ast::BinaryExprKind::Logical) {

            if (!binaryExpr.lValue()->type()->isSameType(&type::ScalarType::Bool)) {

                ast::CastExpr *rValue = ast::CastExpr::createImplicit(binaryExpr.lValue(), &type::ScalarType::Bool);
                binaryExpr.setLValue(rValue);

            }

            if (!binaryExpr.rValue()->type()->isSameType(&type::ScalarType::Bool)) {

                ast::CastExpr *lValue = ast::CastExpr::createImplicit(binaryExpr.rValue(), &type::ScalarType::Bool);
                binaryExpr.setRValue(lValue);

            }
        } else if (!binaryExpr.lValue()->type()->isSameType(binaryExpr.rValue()->type())) {

            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if (binaryExpr.lValue()->type()->canImplicitCastTo(binaryExpr.rValue()->type())
                && binaryExpr.operation() != ast::BinaryOperationKind::Assign) {

                ast::CastExpr *lValue = ast::CastExpr::createImplicit(binaryExpr.lValue(), binaryExpr.rValue()->type());
                binaryExpr.setLValue(lValue);

            } //otherwise, if we can do the reverse, we should...
            else if (binaryExpr.rValue()->type()->canImplicitCastTo(binaryExpr.lValue()->type())) {

                ast::CastExpr *rValue = ast::CastExpr::createImplicit(binaryExpr.rValue(), binaryExpr.lValue()->type());
                binaryExpr.setRValue(rValue);

            } else { // No implicit cast available...
                const char *message = binaryExpr.operation() == ast::BinaryOperationKind::Assign
                                      ? "Cannot assign value of type '%s' to a variable of type '%s'"
                                      : "Cannot implicitly convert '%s' to '%s' or vice-versa";

                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInBinaryExpr,
                    binaryExpr.operatorSpan(),
                    message,
                    binaryExpr.rValue()->type()->nameForDisplay().c_str(),
                    binaryExpr.lValue()->type()->nameForDisplay().c_str());
            }
        }
    }

    void visitedIfExpr(ast::IfExprStmt &ifExpr) override {

        if(!ifExpr.condition()->type()->isSameType(&type::ScalarType::Bool)) {
            if (ifExpr.condition()->type()->canImplicitCastTo(&type::ScalarType::Bool)) {

                ifExpr.setCondition(ast::CastExpr::createImplicit(ifExpr.condition(), &type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfCondition,
                    ifExpr.condition()->sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    ifExpr.condition()->type()->nameForDisplay().c_str());
            }
        }

        if(!ifExpr.elseExpr()) return;

        if(!ifExpr.thenExpr()->type()->isSameType(ifExpr.elseExpr()->type())) {
            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if(ifExpr.elseExpr()->type()->canImplicitCastTo(ifExpr.thenExpr()->type())) {

                ifExpr.setElseExpr(ast::CastExpr::createImplicit(ifExpr.elseExpr(), ifExpr.thenExpr()->type()));

            } //otherwise, if we can to the reverse, we should...
            else if (ifExpr.thenExpr()->type()->canImplicitCastTo(ifExpr.elseExpr()->type())) {

                ifExpr.setThenExpr(ast::CastExpr::createImplicit(ifExpr.thenExpr(), ifExpr.elseExpr()->type()));

            }
            else { // No implicit cast available...
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfBodies,
                    ifExpr.sourceSpan(),
                    "Cannot implicitly cast '%s' to '%s' or vice-versa",
                    ifExpr.thenExpr()->type()->nameForDisplay().c_str(),
                    ifExpr.elseExpr()->type()->nameForDisplay().c_str());
            }
        }
    }

    void visitingWhileExpr(ast::WhileExpr &whileExpr) override {
        //Can we deduplicate this code? (duplicate is in AddImplicitCastsVisitor::visitedIfExpr)
        if(!whileExpr.condition()->type()->isSameType(&type::ScalarType::Bool)) {
            if (whileExpr.condition()->type()->canImplicitCastTo(&type::ScalarType::Bool)) {

                whileExpr.setCondition(ast::CastExpr::createImplicit(whileExpr.condition(), &type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInInWhileCondition,
                    whileExpr.condition()->sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    whileExpr.condition()->type()->nameForDisplay().c_str());
            }
        }
    }

    void visitedFuncDeclStmt(ast::FuncDefStmt &funcDef) override {
        if(funcDef.returnType()->isSameType(&type::ScalarType::Void)) return;

        if(!funcDef.returnType()->isSameType(funcDef.body()->type())) {
            if(!funcDef.body()->type()->canImplicitCastTo(funcDef.returnType())) {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInImplicitReturn,
                    funcDef.body()->sourceSpan(),
                    "Cannot implicitly cast implicit return value from '%s' to '%s'.",
                    funcDef.body()->type()->nameForDisplay().c_str(),
                    funcDef.returnType()->nameForDisplay().c_str());
            } else {
                funcDef.setBody(ast::CastExpr::createImplicit(funcDef.body(), funcDef.returnType()));
            }
        }
    }

    void visitedAssertExprStmt(ast::AssertExprStmt &assertExprStmt) override {

        if(!assertExprStmt.condition()->type()->isSameType(&type::ScalarType::Bool)) {
            if (assertExprStmt.condition()->type()->canImplicitCastTo(&type::ScalarType::Bool)) {

                assertExprStmt.setCondition(ast::CastExpr::createImplicit(assertExprStmt.condition(), &type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInAssertCondition,
                    assertExprStmt.condition()->sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    assertExprStmt.condition()->type()->nameForDisplay().c_str());
            }
        }
    }
};

}}}