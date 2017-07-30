#include "visualize.h"
#include "string.h"

#include <iostream>
#include <algorithm>

namespace lwnn {
    namespace visualize {

        using namespace lwnn::ast;

        class PrettyPrinterVisitor : public AstVisitor {
            IndentWriter writer_;

        public:
            PrettyPrinterVisitor(std::ostream &out) : writer_(out, "  ") { }

            virtual void visitingCompoundStmt(CompoundStmt *expr) override {
                writer_.write("Block: ");
                writeScopeVariables(expr->scope());
                writer_.writeln();
                writer_.incIndent();
            }

            virtual void visitedCompoundStmt(CompoundStmt *expr) override {
                writer_.decIndent();
            }

            void writeScopeVariables(scope::SymbolTable *scope) {
                writer_.write('(');
                auto symbols = scope->symbols();
                if (symbols.size() == 0) {
                    writer_.write(')');
                } else if (symbols.size() == 1) {
                    writer_.write(symbols.front()->toString());
                    writer_.write(')');
                } else {
                    std::sort(symbols.begin(), symbols.end(),
                       [](scope::Symbol *a, scope::Symbol *b) {
                           return a->name() < b->name();
                       });

                    for (auto itr = symbols.begin(); itr != symbols.end() - 1; ++itr) {
                        writer_.write((*itr)->toString());
                        writer_.write(", ");
                    }

                    writer_.write(symbols.back()->toString());
                    writer_.write(')');
                }
            }

            virtual void visitingBinaryExpr(BinaryExpr *expr) override {
                writer_.writeln("BinaryExpr: " + to_string(expr->operation()));
                writer_.incIndent();
            }

            virtual void visitedBinaryExpr(BinaryExpr *expr) override {
                writer_.decIndent();
            }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *expr) override {
                writer_.writeln("LiteralInt32Expr: " + std::to_string(expr->value()));
            }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *expr) override {
                writer_.writeln("LiteralFloat: " + std::to_string(expr->value()));
            }

            virtual void visitingVariableDeclExpr(VariableDeclExpr *expr) override {
                writer_.writeln("VariableDeclExpr: (%s:%s)", expr->name().c_str(),
                                expr->typeRef()->name().c_str());
                writer_.incIndent();
            }

            virtual void visitedVariableDeclExpr(VariableDeclExpr *) override {
                writer_.decIndent();

            }
            virtual void visitVariableRefExpr(VariableRefExpr *expr) override {
                writer_.writeln("VariableRefExpr: " + expr->name());
            }

            virtual bool visitingIfExpr(IfExpr *) override {
                writer_.writeln("Select:");
                writer_.incIndent();
                return true;
            }

            virtual void visitedIfExpr(IfExpr *) override {
                writer_.decIndent();
            }

            virtual void visitingCastExpr(CastExpr *expr) override {
                writer_.writeln("Cast(%s, to %s):",
                                expr->castKind() == CastKind::Implicit ? "implicit" : "explicit",
                                expr->type()->name().c_str());

                writer_.incIndent();
            }
            virtual void visitedCastExpr(CastExpr *pExpr) override {
                writer_.decIndent();
            }

            virtual void visitingReturnStmt(ReturnStmt *) override {
                writer_.writeln("Return:");
                writer_.incIndent();
            }

            virtual void visitedReturnStmt(ReturnStmt *) override {
                writer_.decIndent();
            }

            virtual void visitingFuncDeclStmt(FuncDeclStmt *func) override {
                writer_.writeln("FunctionStmt: " + func->name());
                writer_.incIndent();
            }

            virtual void visitedFuncDeclStmt(FuncDeclStmt *func) override {
                writer_.decIndent();
            }

            virtual void visitingModule(Module *module) override {
                writer_.writeln("Module: " + module->name());
                writer_.incIndent();
            }
        };

        void prettyPrint(Module *module) {
            std::cout << "LWNN AST:\n";
            PrettyPrinterVisitor visitor{ std::cout };
            module->accept(&visitor);
            std::cout << "\n";
        }
    } //namespace ast
} //namespace lwnn