#pragma once

#include "ExpressionTreeVisitor.hpp"

namespace lwnn {
    /** Default tree walker, suitable for most purposes */
    class ExpressionTreeWalker {
        ExpressionTreeVisitor *visitor_;

    public:
        ExpressionTreeWalker(ExpressionTreeVisitor *visitor) : visitor_{visitor} {

        }

        virtual ~ExpressionTreeWalker() {

        }

        void walkTree(const Module *expr) {
            visitor_->initialize();

            this->walkModule(expr);

            visitor_->cleanUp();
        }


        virtual void walk(const Node *node) const  {

            ARG_NOT_NULL(node);
            switch (node->nodeKind()) {
                case NodeKind::LiteralInt32:
                    walkLiteralInt32(static_cast<const LiteralInt32*>(node));
                    break;
                case NodeKind::LiteralFloat:
                    walkLiteralFloat(static_cast<const LiteralFloat*>(node));
                    break;
                case NodeKind::Binary:
                    walkBinary(static_cast<const Binary*>(node));
                    break;
                case NodeKind::Block:
                    walkBlock(static_cast<const Block*>(node));
                    break;
                case NodeKind::Conditional:
                    walkConditional(static_cast<const Conditional*>(node));
                    break;
                case NodeKind::VariableRef:
                    walkVariableRef(static_cast<const VariableRef*>(node));
                    break;
                case NodeKind::AssignVariable:
                    walkAssignVariable(static_cast<const AssignVariable*>(node));
                    break;
                case NodeKind::Return:
                    walkReturn(static_cast<const Return*>(node));
                    break;
                    //TODO:  flesh out visitor for these two
                case NodeKind::Function:
                    walkFunction(static_cast<const Function*>(node));
                    break;
                case NodeKind::Module:
                    walkModule(static_cast<const Module*>(node));
                    break;
                default:
                    throw UnhandledSwitchCase();
            }
        }

        void walkBlock(const Block *blockExpr) const {
            ARG_NOT_NULL(blockExpr);
            visitor_->visitingNode(blockExpr);
            visitor_->visitingBlock(blockExpr);

            blockExpr->forEach([this](const Expr *childExpr) { walk(childExpr); });

            visitor_->visitedBlock(blockExpr);
            visitor_->visitedNode(blockExpr);
        }

        void walkVariableRef(const VariableRef *variableRefExpr) const {
            ARG_NOT_NULL(variableRefExpr);
            visitor_->visitingNode(variableRefExpr);
            visitor_->visitVariableRef(variableRefExpr);
            visitor_->visitedNode(variableRefExpr);
        }

        void walkAssignVariable(const AssignVariable *assignVariableExpr) const {

            ARG_NOT_NULL(assignVariableExpr);
            visitor_->visitingNode(assignVariableExpr);
            visitor_->visitingAssignVariable(assignVariableExpr);

            walk(assignVariableExpr->valueExpr());

            visitor_->visitedAssignVariable(assignVariableExpr);
            visitor_->visitedNode(assignVariableExpr);
        }

        void walkReturn(const Return *returnExpr) const {

            ARG_NOT_NULL(returnExpr);
            visitor_->visitingNode(returnExpr);
            visitor_->visitingReturn(returnExpr);

            walk(returnExpr->valueExpr());

            visitor_->visitedReturn(returnExpr);
            visitor_->visitedNode(returnExpr);
        }

        void walkBinary(const Binary *binaryExpr) const {
            ARG_NOT_NULL(binaryExpr);
            visitor_->visitingNode(binaryExpr);
            visitor_->visitingBinary(binaryExpr);

            walk(binaryExpr->lValue());
            walk(binaryExpr->rValue());

            visitor_->visitedBinary(binaryExpr);
            visitor_->visitedNode(binaryExpr);
        }

        void walkConditional(const Conditional *conditionalExpr) const {
            ARG_NOT_NULL(conditionalExpr);
            visitor_->visitingNode(conditionalExpr);
            visitor_->visitingConditional(conditionalExpr);

            walk(conditionalExpr->condition());
            if (conditionalExpr->truePart()) {
                walk(conditionalExpr->truePart());
            }

            if (conditionalExpr->falsePart()) {
                walk(conditionalExpr->falsePart());
            }

            visitor_->visitedConditional(conditionalExpr);
            visitor_->visitedNode(conditionalExpr);
        }

        void walkLiteralInt32(const LiteralInt32 *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralInt32(expr);
            visitor_->visitedNode(expr);
        }

        void walkLiteralFloat(const LiteralFloat *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralFloat(expr);
            visitor_->visitedNode(expr);
        }

        void walkFunction(const Function *func) const {
            ARG_NOT_NULL(func);
            visitor_->visitingNode(func);
            visitor_->visitingFunction(func);

            walk(func->body());

            visitor_->visitedFunction(func);
            visitor_->visitedNode(func);
        }

        void walkModule(const Module *module) const {
            ARG_NOT_NULL(module);
            visitor_->visitingNode(module);
            visitor_->visitingModule(module);

            module->forEachFunction([this](const Function *func) { walkFunction(func); });

            visitor_->visitedModule(module);
            visitor_->visitedNode(module);
        }

    };

}