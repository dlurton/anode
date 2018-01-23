
#pragma once
#include "ast/ast.h"
#include "front/ErrorStream.h"

namespace anode { namespace front { namespace passes {

class AddImplicitCastsPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit AddImplicitCastsPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    // Note:  due to it being a more natural place for it, implicit casts for function call arguments are done in FuncCallSemanticsPass

    void visitedBinaryExpr(ast::BinaryExprStmt &binaryExpr) override {
        if (binaryExpr.binaryExprKind() == ast::BinaryExprKind::Logical) {

            if (!binaryExpr.lValue().exprType().isSameType(&type::ScalarType::Bool)) {

                ast::CastExprStmtStmt &rValue = ast::CastExprStmtStmt::createImplicit(binaryExpr.lValue(), type::ScalarType::Bool);
                binaryExpr.setLValue(rValue);

            }

            if (!binaryExpr.rValue().exprType().isSameType(&type::ScalarType::Bool)) {

                ast::CastExprStmtStmt &lValue = ast::CastExprStmtStmt::createImplicit(binaryExpr.rValue(), type::ScalarType::Bool);
                binaryExpr.setRValue(lValue);

            }
        } else if (!binaryExpr.lValue().exprType().isSameType(binaryExpr.rValue().exprType())) {

            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if (binaryExpr.lValue().exprType().canImplicitCastTo(binaryExpr.rValue().exprType())
                && binaryExpr.operation() != ast::BinaryOperationKind::Assign) {

                ast::CastExprStmtStmt &lValue = ast::CastExprStmtStmt::createImplicit(binaryExpr.lValue(), binaryExpr.rValue().exprType());
                binaryExpr.setLValue(lValue);

            } //otherwise, if we can do the reverse, we should...
            else if (binaryExpr.rValue().exprType().canImplicitCastTo(binaryExpr.lValue().exprType())) {

                ast::CastExprStmtStmt &rValue = ast::CastExprStmtStmt::createImplicit(binaryExpr.rValue(), binaryExpr.lValue().exprType());
                binaryExpr.setRValue(rValue);

            } else { // No implicit cast available...
                const char *message = binaryExpr.operation() == ast::BinaryOperationKind::Assign
                                      ? "Cannot assign value of type '%s' to a variable of type '%s'"
                                      : "Cannot implicitly convert '%s' to '%s' or vice-versa";

                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInBinaryExpr,
                    binaryExpr.operatorSpan(),
                    message,
                    binaryExpr.rValue().exprType().nameForDisplay().c_str(),
                    binaryExpr.lValue().exprType().nameForDisplay().c_str());
            }
        }
    }

    void visitedIfExpr(ast::IfExprStmt &ifExpr) override {

        if(!ifExpr.condition().exprType().isSameType(&type::ScalarType::Bool)) {
            if (ifExpr.condition().exprType().canImplicitCastTo(&type::ScalarType::Bool)) {

                ifExpr.setCondition(ast::CastExprStmtStmt::createImplicit(ifExpr.condition(), type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfCondition,
                    ifExpr.condition().sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    ifExpr.condition().exprType().nameForDisplay().c_str());
            }
        }

        if(!ifExpr.elseExpr()) return;

        if(!ifExpr.thenExpr().exprType().isSameType(ifExpr.elseExpr()->exprType())) {
            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if(ifExpr.elseExpr()->exprType().canImplicitCastTo(ifExpr.thenExpr().exprType())) {

                ifExpr.setElseExpr(ast::CastExprStmtStmt::createImplicit(*ifExpr.elseExpr(), ifExpr.thenExpr().exprType()));

            } //otherwise, if we can to the reverse, we should...
            else if (ifExpr.thenExpr().exprType().canImplicitCastTo(ifExpr.elseExpr()->exprType())) {

                ifExpr.setThenExpr(ast::CastExprStmtStmt::createImplicit(ifExpr.thenExpr(), ifExpr.elseExpr()->exprType()));

            }
            else { // No implicit cast available...
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfBodies,
                    ifExpr.sourceSpan(),
                    "Cannot implicitly cast '%s' to '%s' or vice-versa",
                    ifExpr.thenExpr().exprType().nameForDisplay().c_str(),
                    ifExpr.elseExpr()->exprType().nameForDisplay().c_str());
            }
        }
    }

    void beforeVisit(ast::WhileExprStmt &whileExpr) override {
        //Can we deduplicate this code? (duplicate is in AddImplicitCastsVisitor::visitedIfExpr)
        if(!whileExpr.condition().exprType().isSameType(&type::ScalarType::Bool)) {
            if (whileExpr.condition().exprType().canImplicitCastTo(&type::ScalarType::Bool)) {

                whileExpr.setCondition(ast::CastExprStmtStmt::createImplicit(whileExpr.condition(), type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInInWhileCondition,
                    whileExpr.condition().sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    whileExpr.condition().exprType().nameForDisplay().c_str());
            }
        }
    }

    void visitedFuncDeclStmt(ast::FuncDefExprStmt &funcDef) override {
        if(funcDef.returnType().isSameType(&type::ScalarType::Void)) return;

        if(!funcDef.returnType().isSameType(funcDef.body().exprType())) {
            if(!funcDef.body().exprType().canImplicitCastTo(funcDef.returnType())) {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInImplicitReturn,
                    funcDef.body().sourceSpan(),
                    "Cannot implicitly cast implicit return value from '%s' to '%s'.",
                    funcDef.body().exprType().nameForDisplay().c_str(),
                    funcDef.returnType().nameForDisplay().c_str());
            } else {
                funcDef.setBody(ast::CastExprStmtStmt::createImplicit(funcDef.body(), funcDef.returnType()));
            }
        }
    }

    void visitedAssertExprStmt(ast::AssertExprStmt &assertExprStmt) override {

        if(!assertExprStmt.condition().exprType().isSameType(&type::ScalarType::Bool)) {
            if (assertExprStmt.condition().exprType().canImplicitCastTo(&type::ScalarType::Bool)) {

                assertExprStmt.setCondition(ast::CastExprStmtStmt::createImplicit(assertExprStmt.condition(), type::ScalarType::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInAssertCondition,
                    assertExprStmt.condition().sourceSpan(),
                    "Cannot implicitly cast condition expression from '%s' to 'bool'.",
                    assertExprStmt.condition().exprType().nameForDisplay().c_str());
            }
        }
    }
};

}}}