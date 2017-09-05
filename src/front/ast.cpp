
#include "front/ast.h"


namespace anode { namespace ast {

unsigned long astNodesDestroyedCount = 0;

std::string to_string(UnaryOperationKind type) {
    switch(type) {
        case UnaryOperationKind::Not:
            return "!";
        case UnaryOperationKind::PreIncrement:
            return "++(pre)";
        case UnaryOperationKind::PreDecrement:
            return "--(pre)";
        default:
            return "<unknown UnaryOperationKind>";
    }
}

std::string to_string(BinaryOperationKind kind) {
    switch (kind) {
        case BinaryOperationKind::Assign:
            return "=";
        case BinaryOperationKind::Add:
            return "+";
        case BinaryOperationKind::Sub:
            return "-";
        case BinaryOperationKind::Mul:
            return "*";
        case BinaryOperationKind::Div:
            return "/";
        case BinaryOperationKind::Eq:
            return "==";
        case BinaryOperationKind::NotEq:
            return "!=";
        case BinaryOperationKind::LogicalAnd:
            return "&&";
        case BinaryOperationKind::LogicalOr:
            return "||";
        case BinaryOperationKind::GreaterThan:
            return ">";
        case BinaryOperationKind::GreaterThanOrEqual:
            return ">=";
        case BinaryOperationKind::LessThan:
            return "<";
        case BinaryOperationKind::LessThanOrEqual:
            return "<=";
        default:
            ASSERT_FAIL("Unhandled BinaryOperationKind");
    }
}

CastExpr *CastExpr::createImplicit(ExprStmt *valueExpr, type::Type *toType) {
    return new CastExpr(valueExpr->sourceSpan(), toType, valueExpr, CastKind::Implicit);
}


type::FunctionType *createFunctionType(type::Type *returnType, const gc_vector<ParameterDef*> &parameters) {
    gc_vector<type::Type*> parameterTypes;
    parameterTypes.reserve(parameters.size());
    for(auto p : parameters) {
        parameterTypes.push_back(p->type());
    }

    return new type::FunctionType(returnType, parameterTypes);
}


}}
