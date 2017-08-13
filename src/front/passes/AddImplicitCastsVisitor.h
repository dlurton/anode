
#pragma once
#include "front/ast.h"
#include "front/error.h"

namespace lwnn { namespace front { namespace passes {

class AddImplicitCastsVisitor : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    AddImplicitCastsVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) {}

    virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) {
        if (binaryExpr->binaryExprKind() == ast::BinaryExprKind::Logical) {
            if (binaryExpr->lValue()->type()->primitiveType() != type::PrimitiveType::Bool) {

                ast::CastExpr *rValue = ast::CastExpr::createImplicit(binaryExpr->lValue(), &type::Primitives::Bool);
                binaryExpr->setLValue(rValue);

            }

            if (binaryExpr->rValue()->type()->primitiveType() != type::PrimitiveType::Bool) {

                ast::CastExpr *lValue = ast::CastExpr::createImplicit(binaryExpr->rValue(), &type::Primitives::Bool);
                binaryExpr->setRValue(lValue);

            }
        } else if (binaryExpr->lValue()->type() != binaryExpr->rValue()->type()) {

            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if (binaryExpr->lValue()->type()->canImplicitCastTo(binaryExpr->rValue()->type())
                && binaryExpr->operation() != ast::BinaryOperationKind::Assign) {

                ast::CastExpr *lValue = ast::CastExpr::createImplicit(binaryExpr->lValue(), binaryExpr->rValue()->type());
                binaryExpr->setLValue(lValue);

            } //otherwise, if we can to the reverse, we should...
            else if (binaryExpr->rValue()->type()->canImplicitCastTo(binaryExpr->lValue()->type())) {

                ast::CastExpr *rValue = ast::CastExpr::createImplicit(binaryExpr->rValue(), binaryExpr->lValue()->type());
                binaryExpr->setRValue(rValue);

            } else { // No implicit cast available...
                const char *message = binaryExpr->operation() == ast::BinaryOperationKind::Assign
                                      ? "Cannot assign value of type '%s' to a variable of type '%s'"
                                      : "Cannot implicitly convert '%s' to '%s' or vice-versa";

                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInBinaryExpr,
                    binaryExpr->operatorSpan(),
                    message,
                    binaryExpr->rValue()->type()->name().c_str(),
                    binaryExpr->lValue()->type()->name().c_str());
            }
        }
    }

    virtual void visitedIfExpr(ast::IfExprStmt *ifExpr) {

        if(ifExpr->condition()->type()->primitiveType() != type::PrimitiveType::Bool) {
            if (ifExpr->condition()->type()->canImplicitCastTo(&type::Primitives::Bool)) {

                ifExpr->setCondition(ast::CastExpr::createImplicit(ifExpr->condition(), &type::Primitives::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfCondition,
                    ifExpr->condition()->sourceSpan(),
                    "Condition expression cannot be implicitly converted from '%s' to 'bool'.",
                    ifExpr->condition()->type()->name().c_str());
            }
        }

        if(!ifExpr->elseExpr()) return;

        if(ifExpr->thenExpr()->type() != ifExpr->elseExpr()->type()) {
            //If we can implicitly cast the lvalue to same type as the rvalue, we should...
            if(ifExpr->elseExpr()->type()->canImplicitCastTo(ifExpr->thenExpr()->type())) {

                ifExpr->setElseExpr(ast::CastExpr::createImplicit(ifExpr->elseExpr(), ifExpr->thenExpr()->type()));

            } //otherwise, if we can to the reverse, we should...
            else if (ifExpr->thenExpr()->type()->canImplicitCastTo(ifExpr->elseExpr()->type())) {

                ifExpr->setThenExpr(ast::CastExpr::createImplicit(ifExpr->thenExpr(), ifExpr->elseExpr()->type()));
            }
            else { // No implicit cast available...
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInIfBodies,
                    ifExpr->sourceSpan(),
                    "Cannot implicitly convert '%s' to '%s' or vice-versa",
                    ifExpr->thenExpr()->type()->name().c_str(),
                    ifExpr->elseExpr()->type()->name().c_str());
            }
        }
    }

    virtual bool visitingWhileExpr(ast::WhileExpr *whileExpr) {
        //Can we deduplicate this code? (duplicate is in AddImplicitCastsVisitor::visitedIfExpr)
        if(whileExpr->condition()->type()->primitiveType() != type::PrimitiveType::Bool) {
            if (whileExpr->condition()->type()->canImplicitCastTo(&type::Primitives::Bool)) {

                whileExpr->setCondition(ast::CastExpr::createImplicit(whileExpr->condition(), &type::Primitives::Bool));

            } else {
                errorStream_.error(
                    error::ErrorKind::InvalidImplicitCastInInWhileCondition,
                    whileExpr->condition()->sourceSpan(),
                    "Condition expression cannot be implicitly converted from '%s' to 'bool'.",
                    whileExpr->condition()->type()->name().c_str());
            }
        }

        return true;
    }
};

}}}