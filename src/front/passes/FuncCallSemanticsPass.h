
#pragma once

#include "ErrorContextAstVisitor.h"

namespace anode { namespace front  { namespace passes {


class FuncCallSemanticsPass : public ErrorContextAstVisitor {
public:
    explicit FuncCallSemanticsPass(error::ErrorStream &errorStream_) : ErrorContextAstVisitor(errorStream_) { }

    void visitedFuncCallExprStmt(ast::FuncCallExprStmt &funcCallExpr) override {
        if(!funcCallExpr.funcExpr().exprType().isFunction()) {
            errorStream_.error(
                error::ErrorKind::ExpressionIsNotFunction,
                funcCallExpr.sourceSpan(),
                "Result of expression left of '(' is not a function.");
            return;
        }

        auto *funcType = dynamic_cast<type::FunctionType*>(&funcCallExpr.funcExpr().exprType());
        ASSERT(funcType);

        //When we do function overloading, this is going to get a whole lot more complicated.
        auto parameterTypes = funcType->parameterTypes();
        auto arguments = funcCallExpr.arguments();
        if(parameterTypes.size() != arguments.size()) {
            errorStream_.error(
                error::ErrorKind::IncorrectNumberOfArguments,
                funcCallExpr.sourceSpan(),
                "Incorrect number of arguments.  Expected %d but found %d",
                parameterTypes.size(),
                arguments.size());
            return;
        }

        for(size_t i = 0; i < arguments.size(); ++i) {
            auto &argument = arguments[i].get();
            auto &parameterType = parameterTypes[i].get();

            if(!parameterType.isSameType(argument.exprType())) {
                if(!argument.exprType().canImplicitCastTo(parameterType)) {
                    errorStream_.error(
                        error::ErrorKind::InvalidImplicitCastInFunctionCallArgument,
                        argument.sourceSpan(),
                        "Cannot implicitly cast argument %d from '%s' to '%s'.",
                        i,
                        argument.exprType().nameForDisplay().c_str(),
                        parameterType.nameForDisplay().c_str());
                } else {
                    auto &implicitCast = ast::CastExprStmtStmt::createImplicit(argument, parameterType);
                    funcCallExpr.replaceArgument(i, implicitCast);
                }
            }
        }
    }
};

}}}