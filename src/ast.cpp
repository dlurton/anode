
#include "ast.h"


namespace lwnn {
    namespace ast {

        std::string to_string(StmtKind kind) {
            switch (kind) {
                case StmtKind::CompoundStmt:
                    return "CompoundStmt";
                case StmtKind::FunctionDeclStmt:
                    return "FunctionDeclStmt";
                case StmtKind::ReturnStmt:
                    return "ReturnStmt";
                case StmtKind::ExprStmt:
                    return "ExprStmt";
                default:
                    ASSERT_FAIL("Unhandled StmtKind");
            }
        }

        bool isArithmeticOperation(BinaryOperationKind kind) {
            return (int)kind > (int)BinaryOperationKind::ArithmeticBeginValue
                   && (int)kind < (int)BinaryOperationKind::ArithmeticEndValue;
        }

        std::string to_string(ExprKind kind) {
            switch (kind) {
                case ExprKind::VariableDeclExpr:
                    return "VariableDeclExpr";
                case ExprKind::LiteralInt32Expr:
                    return "LiteralInt32Expr";
                case ExprKind::LiteralFloatExpr:
                    return "LiteralFloatExpr";
                case ExprKind::VariableRefExpr:
                    return "VariableRefExpr";
                case ExprKind::BinaryExpr:
                    return "BinaryExpr";
                case ExprKind::ConditionalExpr:
                    return "ConditionalExpr";
                default:
                    ASSERT_FAIL("Unhandled ExprKind");
            }
        }

        std::string to_string(BinaryOperationKind type) {
            switch (type) {
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
                default:
                    ASSERT_FAIL("Unhandled BinaryOperationKind");
            }
        }

    } //namespace ast
} //namespace lwnn
