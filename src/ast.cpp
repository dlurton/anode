
#include "ast.h"


namespace lwnn {

    namespace ast {
        SourceSpan SourceSpan::Any("?", SourceLocation(-1, -1), SourceLocation(-1, -1));

        std::string to_string(NodeKind nodeKind) {
            switch (nodeKind) {
                case NodeKind::Binary:
                    return "Binary";
                case NodeKind::Invoke:
                    return "Invoke";
                case NodeKind::VariableRef:
                    return "Variable";
                case NodeKind::AssignVariable:
                    return "AssignVariable";
                case NodeKind::Conditional:
                    return "Conditional";
                case NodeKind::Switch:
                    return "Switch";
                case NodeKind::Block:
                    return "Block";
                case NodeKind::LiteralInt32:
                    return "LiteralInt";
                default:
                    throw exception::UnhandledSwitchCase();
            }
        }

        std::string to_string(OperationKind type) {
            switch (type) {
                case OperationKind::Add:
                    return "Add";
                case OperationKind::Sub:
                    return "Sub";
                case OperationKind::Mul:
                    return "Mul";
                case OperationKind::Div:
                    return "Div";
                default:
                    throw exception::UnhandledSwitchCase();
            }
        }

        std::string to_string(DataType dataType) {
            switch (dataType) {
                case DataType::Void:
                    return "void";
                case DataType::Bool:
                    return "Bool";
                case DataType::Int32:
                    return "Int32";
                case DataType::Float:
                    return "Float";
                case DataType::Pointer:
                    return "Pointer";
                default:
                    throw exception::UnhandledSwitchCase();
            }
        }
    } //namespace ast
} //namespace lwnn
