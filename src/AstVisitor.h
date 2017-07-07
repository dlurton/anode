
#pragma once

#include "ast.h"

namespace lwnn {

    /** The base class providing virtual methods with empty default implementations for all visitors.
     * Node types which do not have children will only have an overload of visit(...);  Node types that do have
     * children will have overloads for visiting(...) and visited(...) member functions. */
    class AstVisitor {
    public:

        virtual void initialize() {}

        virtual void cleanUp() {}

        /** Executes before every node is visited. */
        virtual void visitingNode(const AstNode *) { }

        /** Executes after every node is visited. */
        virtual void visitedNode(const AstNode *) {}

        virtual void visitingBlock(const BlockExpr *) {}

        virtual void visitedBlock(const BlockExpr *) {}

        virtual void visitingConditional(const ConditionalExpr *) {}

        virtual void visitedConditional(const ConditionalExpr *) {}

        virtual void visitingBinary(const BinaryExpr *) {}

        virtual void visitedBinary(const BinaryExpr *) {}

        virtual void visitLiteralInt32(const LiteralInt32Expr *) {}
        virtual void visitLiteralFloat(const LiteralFloatExpr *) {}

        virtual void visitingReturn(const Return *) {}
        virtual void visitedReturn(const Return *) {}

        virtual void visitVariableRef(const VariableRef *) {}

        virtual void visitingAssignVariable(const AssignVariable *) {}

        virtual void visitedAssignVariable(const AssignVariable *) {}

        virtual void visitingFunction(const Function *) {}
        virtual void visitedFunction(const Function *) {}

        virtual void visitingModule(const Module *) {}
        virtual void visitedModule(const Module *) {}

    };

}