
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
                case ExprKind::AssignVariableExpr:
                    return "AssignVariableExpr";
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
                case BinaryOperationKind::Add:
                    return "Add";
                case BinaryOperationKind::Sub:
                    return "Sub";
                case BinaryOperationKind::Mul:
                    return "Mul";
                case BinaryOperationKind::Div:
                    return "Div";
                default:
                    ASSERT_FAIL("Unhandled BinaryOperationKind");
            }
        }

    } //namespace ast
} //namespace lwnn
