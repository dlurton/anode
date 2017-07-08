#include "PrettyPrinter.h"
#include "AstWalker.h"

#include <iostream>
#include <algorithm>

namespace lwnn {
    class PrettyPrinterVisitor : public AstVisitor {
    private:
        int indent_ = -1;
        std::ostream &out_;

        void writeTabs() {
            for (int i = 0; i < indent_; ++i)
                out_ << "  ";
        }

    public:

        PrettyPrinterVisitor(std::ostream &out) : out_(out) {}

        ~PrettyPrinterVisitor() {}

        void cleanUp() override {
            out_ << "\n";
        }

        void visitingNode(const AstNode *) override {
            indent_++;
            out_ << "\n";
            writeTabs();
        }

        void visitedNode(const AstNode *) override {
            indent_--;
        }

        void visitingBlock(const BlockExpr *expr) override {
            out_ << "Block:";
            writeScopeVariables(expr->scope());
        }

        void writeScopeVariables(const Scope *scope) {
            out_ << "(";
            std::vector<const VariableDef *> variables = scope->variables();
            if (variables.size() == 0) {
                out_ << ")";
            } else if (variables.size() == 1) {
                out_ << variables.front()->toString() << ")";
            } else {
                std::sort(variables.begin(), variables.end(),
                          [](const VariableDef *a, const VariableDef *b) {
                              return a->name() < b->name();
                          });

                for (auto itr = variables.begin(); itr != variables.end() - 1; ++itr) {
                    out_ << (*itr)->toString() << ", ";
                }

                out_ << variables.back()->toString() << ")";
            }
        }

        void visitingBinary(const BinaryExpr *expr) override {
            out_ << "Binary: " << to_string(expr->operation());
        }

        void visitLiteralInt32(const LiteralInt32Expr *expr) override {
            out_ << "LiteralInt32: " << std::to_string(expr->value());
        }

        void visitLiteralFloat(const LiteralFloatExpr *expr) override {
            out_ << "LiteralFloat: " << std::to_string(expr->value());
        }

        void visitVariableRef(const VariableRef *expr) override {
            out_ << "VariableRef: " << expr->name();
        }

        void visitingConditional(const ConditionalExpr *) override {
            out_ << "Conditional: ";
        }

        virtual void visitingAssignVariable(const AssignVariable *expr) override {
            out_ << "AssignVariable: " << expr->name();
        }

        virtual void visitingReturn(const Return *) override {
            out_ << "Return: ";
        }

        virtual void visitingFunction(const Function *func) override {
            out_ << "Function: " << func->name();
        }

        virtual void visitingModule(const Module *module) override {
            out_ << "Module: " << module->name();
        }
    };

    void prettyPrint(const Module *module) {
        lwnn::PrettyPrinterVisitor visitor{std::cout};
        AstWalker walker{&visitor};
        walker.walkTree(module);
        std::cout << "\n";
    }
}