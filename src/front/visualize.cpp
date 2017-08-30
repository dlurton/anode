#include "front/visualize.h"
#include "common/string_format.h"

#include <iostream>
#include <algorithm>

namespace lwnn { namespace visualize {

using namespace lwnn::ast;

class PrettyPrinterVisitor : public AstVisitor {
    IndentWriter writer_;

public:
    PrettyPrinterVisitor(std::ostream &out) : writer_(out, "  ") { }

    bool visitingCompoundExpr(CompoundExpr *expr) override {
        writer_.write("CompoundExpr: ");
        writeScopeVariables(expr->scope());
        writer_.writeln();
        writer_.incIndent();
        return true;
    }

    void visitedCompoundExpr(CompoundExpr *) override {
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

    bool visitingBinaryExpr(BinaryExpr *expr) override {
        writer_.writeln("BinaryExpr: " + to_string(expr->operation()));
        writer_.incIndent();
        return true;
    }

    void visitedBinaryExpr(BinaryExpr *) override {
        writer_.decIndent();
    }

    bool visitingUnaryExpr(UnaryExpr *expr) override {
        writer_.writeln("UnaryExpr: " + to_string(expr->operation()) + ", " + expr->type()->name());
        writer_.incIndent();
        return true;
    }

    void visitedUnaryExpr(UnaryExpr *) override {
        writer_.decIndent();
    }

    void visitingDotExpr(DotExpr *expr) override {
        writer_.writeln("DotExpr: " + expr->memberName());
        writer_.incIndent();
        //return true;
    }

    void visitedDotExpr(DotExpr *) override {
        writer_.decIndent();
    }

    void visitLiteralBoolExpr(LiteralBoolExpr *expr) override {
        writer_.writeln("LiteralBoolExpr: %s", expr->value() ? "true" : "false");
    }

    void visitLiteralInt32Expr(LiteralInt32Expr *expr) override {
        writer_.writeln("LiteralInt32Expr: " + std::to_string(expr->value()));
    }

    void visitLiteralFloatExpr(LiteralFloatExpr *expr) override {
        writer_.writeln("LiteralFloat: " + std::to_string(expr->value()));
    }

    void visitingVariableDeclExpr(VariableDeclExpr *expr) override {
        writer_.writeln("VariableDeclExpr: (%s:%s)", expr->name().c_str(),
                        expr->typeRef()->name().c_str());
        writer_.incIndent();
    }

    void visitedVariableDeclExpr(VariableDeclExpr *) override {
        writer_.decIndent();

    }
    void visitVariableRefExpr(VariableRefExpr *expr) override {
        writer_.writeln("VariableRefExpr: " + expr->name());
    }

    bool visitingIfExpr(IfExprStmt *) override {
        writer_.writeln("IfExprStmt:");
        writer_.incIndent();
        return true;
    }

    void visitedIfExpr(IfExprStmt *) override {
        writer_.decIndent();
    }

    bool visitingWhileExpr(WhileExpr *) override {
        writer_.writeln("WhileExpr:");
        writer_.incIndent();
        return true;
    }

    void visitedWhileExpr(WhileExpr *) override {
        writer_.decIndent();
    }


    bool visitingAssertExprStmt(AssertExprStmt *) override {
        writer_.writeln("AssertExprStmt:");
        writer_.incIndent();
        return true;
    }

    void visitedAssertExprStmt(AssertExprStmt *) override {
        writer_.decIndent();
    }

    void visitingCastExpr(CastExpr *expr) override {
        writer_.writeln("CastExpr(%s):",
                        expr->castKind() == CastKind::Implicit ? "implicit" : "explicit");

        writer_.incIndent();
    }
    void visitedCastExpr(CastExpr *) override {
        writer_.decIndent();
    }

    void visitingReturnStmt(ReturnStmt *) override {
        writer_.writeln("ReturnStmt:");
        writer_.incIndent();
    }

    void visitedReturnStmt(ReturnStmt *) override {
        writer_.decIndent();
    }

    void visitingParameterDef(ParameterDef *pd) override {
        writer_.write("ParameterDef: ");
        writer_.write(pd->name());
        writer_.write(':');
        writer_.writeln(pd->typeRef()->name());
    }

    bool visitingFuncDefStmt(FuncDefStmt *func) override {
        writer_.writeln("FuncDefStmt: " + func->name());
//                auto parameters = func->parameters();
//                for(auto p : parameters) {
//                    if(p != parameters.front()) {
//                        writer_.write(", ");
//                    }
//                    writer_.write();
//                }
        writer_.incIndent();
        return true;
    }

    void visitingFuncCallExpr(FuncCallExpr *) override {
        writer_.writeln("FuncCallExpr:");
        writer_.incIndent();
    }

    void visitedFuncCallExpr(FuncCallExpr *) override {
        writer_.decIndent();
    }

    void visitedFuncDeclStmt(FuncDefStmt *) override {
        writer_.decIndent();
    }

    bool visitingClassDefinition(ClassDefinition *ClassDefinition) override {
        writer_.writeln("ClassDefinition: " + ClassDefinition->name());
        writer_.incIndent();

        return true;
    }

    void visitedClassDefinition(ClassDefinition *) override {
        writer_.decIndent();
    }

    bool visitingModule(Module *module) override {
        writer_.writeln("Module: " + module->name());
        writer_.incIndent();

        return true;
    }

    void visitedModule(Module *) override {
        writer_.decIndent();
    }
};

void prettyPrint(AstNode *module) {
    std::cerr<< "LWNN AST:\n";
    PrettyPrinterVisitor visitor{ std::cerr };
    module->accept(&visitor);
    std::cerr << "\n";
}

}}