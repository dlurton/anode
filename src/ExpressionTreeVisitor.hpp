
#pragma once

#include "AST.hpp"

namespace lwnn {

    /** The base class providing virtual methods with empty default implementations for all visitors.
     * Node types which do not have children will only have an overload of visit(...);  Node types that do have
     * children will have overloads for visiting(...) and visited(...) member functions.
     */
    class ExpressionTreeVisitor {
    public:

        virtual void initialize() {}

        virtual void cleanUp() {}

        /** Executes before every node is visited. */
        virtual void visitingNode(const Node *) { }

        /** Executes after every node is visited. */
        virtual void visitedNode(const Node *) {}

        virtual void visitingBlock(const Block *) {}

        virtual void visitedBlock(const Block *) {}

        virtual void visitingConditional(const Conditional *) {}

        virtual void visitedConditional(const Conditional *) {}

        virtual void visitingBinary(const Binary *) {}

        virtual void visitedBinary(const Binary *) {}

        virtual void visitLiteralInt32(const LiteralInt32 *) {}
        virtual void visitLiteralFloat(const LiteralFloat *) {}

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