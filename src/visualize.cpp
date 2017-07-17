#include "visualize.h"
#include "string.h"

#include <iostream>
#include <algorithm>

namespace lwnn {
    namespace visualize {
        class IndentWriter {
            std::ostream &out_;
            int indent_ = 0;
            std::string indentString_;
            bool needsIndent_ = false;
        public:
            IndentWriter(std::ostream &out, std::string indentString) : out_(out), indentString_(indentString) { }

            //Other "write" methods must all directly or indirectly call this in order for indentation to work
            void write(char c) {
                if(needsIndent_) {
                    for(int i = 0; i < indent_; ++i) {
                        out_ << indentString_;
                    }
                    needsIndent_ = false;
                }
                out_ << c;
                if(c == '\n') {
                    needsIndent_ = true;
                }
            }

            void writeln(char c) {
                write(c);
                write('\n');
            }

            template<typename ... Args>
            void write( const std::string& format, Args ... args ) {
                std::string text = string::format(format, args ...);
                for(char c : text) {
                    write(c);
                }
            }

            void writeln() {
                write('\n');
            }

            template<typename ... Args>
            void writeln( const std::string& format, Args ... args ) {
                write(format, args ...);
                write('\n');
            };


            void incIndent() { indent_++; }
            void decIndent() {
                indent_--;
                ASSERT(indent_ >= 0);
            }

        };

        using namespace lwnn::ast;

        class PrettyPrinterVisitor : public AstVisitor {
            IndentWriter writer_;

        public:
            PrettyPrinterVisitor(std::ostream &out) : writer_(out, "  ") { }

            virtual bool visitingCompoundStmt(CompoundStmt *expr) override {
                writer_.write("Block: ");
                writeScopeVariables(expr->scope());
                writer_.writeln();
                writer_.incIndent();
                return true;
            }

            virtual void visitedCompoundStmt(CompoundStmt *expr) override {
                writer_.decIndent();
            }

            void writeScopeVariables(scope::SymbolTable *scope) {
                writer_.write('(');
                auto symbols = scope->symbols();
                if (symbols.size() == 0) {
                    writer_.writeln(')');
                } else if (symbols.size() == 1) {
                    writer_.write(symbols.front()->toString());
                    writer_.writeln(')');
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
                    writer_.writeln(')');
                }
            }

            virtual bool visitingBinaryExpr(BinaryExpr *expr) override {
                writer_.writeln("BinaryExpr: " + to_string(expr->operation()));
                writer_.incIndent();
                return true;
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

            virtual bool visitingVariableDeclExpr(VariableDeclExpr *expr) override {
                writer_.writeln("VariableDeclExpr: (%s:%s)", expr->name().c_str(),
                                expr->typeRef()->name().c_str());
                writer_.incIndent();
                return true;
            }

            virtual void visitedVariableDeclExpr(VariableDeclExpr *) override {
                writer_.decIndent();

            }
            virtual void visitVariableRefExpr(VariableRefExpr *expr) override {
                writer_.writeln("VariableRefExpr: " + expr->name());
            }

            virtual bool visitingConditionalExpr(ConditionalExpr *) override {
                writer_.writeln("Conditional:");
                writer_.incIndent();
                return true;
            }

            virtual void visitedConditionalExpr(ConditionalExpr *) override {
                writer_.decIndent();
            }

            virtual bool visitingAssignExpr(AssignExpr *expr) override {
                writer_.writeln("AssignExpr:");
                writer_.incIndent();
                return true;
            }

            virtual void visitedAssignExpr(AssignExpr *) override {
                writer_.decIndent();
            }

            virtual bool visitingReturnStmt(ReturnStmt *) override {
                writer_.writeln("Return:");
                writer_.incIndent();
                return true;
            }

            virtual void visitedReturnStmt(ReturnStmt *) override {
                writer_.decIndent();
            }

            virtual bool visitingFuncDeclStmt(FuncDeclStmt *func) override {
                writer_.writeln("FunctionStmt: " + func->name());
                writer_.incIndent();
                return true;
            }

            virtual void visitedFuncDeclStmt(FuncDeclStmt *func) override {
                writer_.decIndent();
            }

            virtual bool visitingModule(Module *module) override {
                writer_.writeln("Module: " + module->name());
                return true;
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