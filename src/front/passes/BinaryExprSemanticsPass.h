
#pragma once

namespace anode { namespace front  { namespace passes {

class BinaryExprSemanticsPass : public ErrorContextAstVisitor {
public:
    explicit BinaryExprSemanticsPass(error::ErrorStream &errorStream_) : ErrorContextAstVisitor(errorStream_) { }

    void visitedBinaryExpr(ast::BinaryExpr &binaryExpr) override {
        if(binaryExpr.isComparison()) {
            return;
        }
        if(binaryExpr.operation() == ast::BinaryOperationKind::Assign) {
            if(!binaryExpr.lValue().canWrite()) {
                errorStream_.error(
                    error::ErrorKind::CannotAssignToLValue,
                    binaryExpr.operatorSpan(), "Cannot assign a value to the expression left of '='");
            }
        } else if(binaryExpr.binaryExprKind() == ast::BinaryExprKind::Arithmetic) {
            if(!binaryExpr.type().canDoArithmetic()) {
                errorStream_.error(
                    error::ErrorKind::OperatorCannotBeUsedWithType,
                    binaryExpr.operatorSpan(),
                    "Operator '%s' cannot be used with type '%s'.",
                    ast::to_string(binaryExpr.operation()).c_str(),
                    binaryExpr.type().nameForDisplay().c_str());
            }
        }
    }
};

}}}