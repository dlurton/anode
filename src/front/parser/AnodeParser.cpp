#include "AnodeParser.h"

namespace anode { namespace front { namespace parser {

Associativity AnodeParser::getOperatorAssociativity(TokenKind kind) {
    switch(kind) {
        case TokenKind::OP_DOT:
        case TokenKind::OP_MUL:
        case TokenKind::OP_DIV:
        case TokenKind::OP_ADD:
        case TokenKind::OP_SUB:
        case TokenKind::OP_GT:
        case TokenKind::OP_LT:
        case TokenKind::OP_LTE:
        case TokenKind::OP_GTE:
        case TokenKind::OP_EQ:
        case TokenKind::OP_NEQ:
        case TokenKind::OP_LAND:
        case TokenKind::OP_LOR:
            return Associativity::Left;
        case TokenKind::OP_NOT:
        case TokenKind::OP_ASSIGN:
            return Associativity::Right;
        default:
            ASSERT_FAIL("Unknown operator associativity");
    }
}

int AnodeParser::getOperatorPrecedence(TokenKind kind) {

    //The reason we subtract precedences from MAX_PRECEDENCE is so that
    //we can use the same values from the table here:
    //http://en.cppreference.com/w/cpp/language/operator_precedence
    //...but reverse their ordering as required by the Pratt parsing algorithm.
    const int MAX_PRECEDENCE = 16;
    switch(kind) {
        case TokenKind::OP_DOT:
        case TokenKind::OPEN_PAREN:
            return MAX_PRECEDENCE - 2;
        case TokenKind::OP_NOT:
            return MAX_PRECEDENCE - 3;
        case TokenKind::OP_MUL:
        case TokenKind::OP_DIV:
            return MAX_PRECEDENCE - 5;
        case TokenKind::OP_ADD:
        case TokenKind::OP_SUB:
            return MAX_PRECEDENCE - 6;
        case TokenKind::OP_GT:
        case TokenKind::OP_LT:
        case TokenKind::OP_LTE:
        case TokenKind::OP_GTE:
            return MAX_PRECEDENCE - 8;
        case TokenKind::OP_EQ:
        case TokenKind::OP_NEQ:
            return MAX_PRECEDENCE - 9;
        case TokenKind::OP_LAND:
            return MAX_PRECEDENCE - 13;
        case TokenKind::OP_LOR:
            return MAX_PRECEDENCE - 14;
        case TokenKind::OP_ASSIGN:
            return MAX_PRECEDENCE - 15;
        default:
            return -100000;
    }
}

ast::ExprStmt &AnodeParser::parseBinaryExpr(ast::ExprStmt &lValue, Token &operatorToken) {
    ast::BinaryOperationKind opKind;

    switch(operatorToken.kind()) {
        case TokenKind::OP_ASSIGN: {
            opKind = ast::BinaryOperationKind::Assign;
            ast::VariableRefExpr *varRef = dynamic_cast<ast::VariableRefExpr*>(&lValue);
            if(varRef != nullptr) {
                varRef->setVariableAccess(ast::VariableAccess::Write);
            }
            break;
        }
        case TokenKind::OP_ADD:    opKind = ast::BinaryOperationKind::Add; break;
        case TokenKind::OP_SUB:    opKind = ast::BinaryOperationKind::Sub; break;
        case TokenKind::OP_MUL:    opKind = ast::BinaryOperationKind::Mul; break;
        case TokenKind::OP_DIV:    opKind = ast::BinaryOperationKind::Div; break;
        case TokenKind::OP_EQ:     opKind = ast::BinaryOperationKind::Eq; break;
        case TokenKind::OP_NEQ:    opKind = ast::BinaryOperationKind::NotEq; break;
        case TokenKind::OP_LAND:   opKind = ast::BinaryOperationKind::LogicalAnd; break;
        case TokenKind::OP_LOR:    opKind = ast::BinaryOperationKind::LogicalOr; break;
        case TokenKind::OP_GT:     opKind = ast::BinaryOperationKind::GreaterThan; break;
        case TokenKind::OP_LT:     opKind = ast::BinaryOperationKind::LessThan; break;
        case TokenKind::OP_GTE:    opKind = ast::BinaryOperationKind::GreaterThanOrEqual; break;
        case TokenKind::OP_LTE:    opKind = ast::BinaryOperationKind::LessThanOrEqual; break;

        default:
            ASSERT_FAIL("Unhandled Token Type (Operators)");
    }

    int precedence = getOperatorPrecedence(operatorToken.kind());
    if(getOperatorAssociativity(operatorToken.kind()) == Associativity::Right)
        precedence -= 1;

    ast::ExprStmt &rValue = parseExpr(precedence);

    return *new ast::BinaryExpr(
        makeSourceSpan(lValue.sourceSpan(), rValue.sourceSpan()),
        lValue,
        opKind,
        operatorToken.span(),
        rValue
    );
}

}}}