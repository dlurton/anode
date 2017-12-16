#include "front/visualize.h"
#include "common/string.h"

#include <iostream>
#include <algorithm>

namespace anode { namespace front { namespace visualize {

using namespace anode::front::ast;

class PrettyPrinterVisitor : public AstVisitor {
    IndentWriter writer_;

public:
    explicit PrettyPrinterVisitor(std::ostream &out) : writer_(out, "  ") { }

    void visitingCompoundExpr(CompoundExpr &expr) override {
        writer_.write("CompoundExpr: ");
        writeScopeVariables(expr.scope());
        writer_.writeln();
        writer_.incIndent();
    }

    void visitedCompoundExpr(CompoundExpr &) override {
        writer_.decIndent();
    }

    void writeScopeVariables(scope::SymbolTable &scope) {
        writer_.write('(');
        auto symbols = scope.symbols();
        if (symbols.empty()) {
            writer_.write(')');
        } else if (symbols.size() == 1) {
            writer_.write(symbols.front().get().toString());
            writer_.write(')');
        } else {
            std::sort(symbols.begin(), symbols.end(),
               [](scope::Symbol &a, scope::Symbol &b) {
                   return a.name() < b.name();
               });

            for (auto itr = symbols.begin(); itr != symbols.end() - 1; ++itr) {
                writer_.write((*itr).get().toString());
                writer_.write(", ");
            }

            writer_.write(symbols.back().get().toString());
            writer_.write(')');
        }
    }

    void visitingBinaryExpr(BinaryExpr &expr) override {
        writer_.writeln("BinaryExpr: " + to_string(expr.operation()));
        writer_.incIndent();
    }

    void visitedBinaryExpr(BinaryExpr &) override {
        writer_.decIndent();
    }

    void visitingUnaryExpr(UnaryExpr &expr) override {
        writer_.writeln("UnaryExpr: " + to_string(expr.operation()) + ", " + expr.exprType().nameForDisplay());
        writer_.incIndent();
    }

    void visitedUnaryExpr(UnaryExpr &) override {
        writer_.decIndent();
    }

    void visitingDotExpr(DotExpr &expr) override {
        writer_.writeln("DotExpr: " + expr.memberName().text());
        writer_.incIndent();
        //return true;
    }

    void visitedDotExpr(DotExpr &) override {
        writer_.decIndent();
    }

    void visitLiteralBoolExpr(LiteralBoolExpr &expr) override {
        writer_.writeln("LiteralBoolExpr: %s", expr.value() ? "true" : "false");
    }

    void visitLiteralInt32Expr(LiteralInt32Expr &expr) override {
        writer_.writeln("LiteralInt32Expr: " + std::to_string(expr.value()));
    }

    void visitLiteralFloatExpr(LiteralFloatExpr &expr) override {
        writer_.writeln("LiteralFloat: " + std::to_string(expr.value()));
    }

    void visitingVariableDeclExpr(VariableDeclExpr &expr) override {
        writer_.writeln("VariableDeclExpr: (%s:%s)", expr.name().qualifedName().c_str(),
                        expr.typeRef().name().qualifedName().c_str());
        writer_.incIndent();
    }

    void visitedVariableDeclExpr(VariableDeclExpr &) override {
        writer_.decIndent();

    }
    void visitVariableRefExpr(VariableRefExpr &expr) override {
        writer_.writeln("VariableRefExpr: " + expr.name().qualifedName());
    }

    void visitingIfExpr(IfExprStmt &) override {
        writer_.writeln("IfExprStmt:");
        writer_.incIndent();
    }

    void visitedIfExpr(IfExprStmt &) override {
        writer_.decIndent();
    }

    void visitingWhileExpr(WhileExpr &) override {
        writer_.writeln("WhileExpr:");
        writer_.incIndent();
    }

    void visitedWhileExpr(WhileExpr &) override {
        writer_.decIndent();
    }


    void visitingAssertExprStmt(AssertExprStmt &) override {
        writer_.writeln("AssertExprStmt:");
        writer_.incIndent();
    }

    void visitedAssertExprStmt(AssertExprStmt &) override {
        writer_.decIndent();
    }

    void visitingCastExpr(CastExpr &expr) override {
        writer_.writeln("CastExpr(%s):",
                        expr.castKind() == CastKind::Implicit ? "implicit" : "explicit");

        writer_.incIndent();
    }

    void visitingNewExpr(NewExpr &expr) override {
        writer_.writeln("NewExpr(%s):", expr.typeRef().name().qualifedName().c_str());

        writer_.incIndent();
    }

    void visitedCastExpr(CastExpr &) override {
        writer_.decIndent();
    }

//    void visitingReturnStmt(ReturnStmt &) override {
//        writer_.writeln("ReturnStmt:");
//        writer_.incIndent();
//    }
//
//    void visitedReturnStmt(ReturnStmt &) override {
//        writer_.decIndent();
//    }

    void visitingParameterDef(ParameterDef &pd) override {
        writer_.write("ParameterDef: ");
        writer_.write(pd.name().text());
        writer_.write(':');
        writer_.writeln(pd.typeRef().name().qualifedName().c_str());
    }

    void visitingFuncDefStmt(FuncDefStmt &func) override {
        writer_.writeln("FuncDefStmt: " + func.name().text());
//                auto parameters = func.parameters();
//                for(auto p : parameters) {
//                    if(p != parameters.front()) {
//                        writer_.write(", ");
//                    }
//                    writer_.write();
//                }
        writer_.incIndent();
    }

    void visitingFuncCallExpr(FuncCallExpr &) override {
        writer_.writeln("FuncCallExpr:");
        writer_.incIndent();
    }

    void visitedFuncCallExpr(FuncCallExpr &) override {
        writer_.decIndent();
    }

    void visitedFuncDeclStmt(FuncDefStmt &) override {
        writer_.decIndent();
    }

    void visitingCompleteClassDefinition(CompleteClassDefinition &cd) override {
        writer_.writeln("ClassDefinition: " + cd.name().text());
        writer_.incIndent();
    }

    void visitedCompleteClassDefinition(CompleteClassDefinition &) override {
        writer_.decIndent();
    }

    void visitingGenericClassDefinition(GenericClassDefinition &cd) override {
        writer_.writeln("ClassDefinition: " + cd.name().text());
        writer_.incIndent();
    }

    void visitedGenericClassDefinition(GenericClassDefinition &) override {
        writer_.decIndent();
    }
    

    virtual void visitingExpressionList(ExpressionList &) override {
        writer_.writeln("ExpressionList");
        writer_.incIndent();
    }
    virtual void visitedExpressionList(ExpressionList &) override {
        writer_.decIndent();
    }

    virtual void visitingAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &) override {
        writer_.writeln("AnonymousTemplateExprStmt");
        writer_.incIndent();
    }

    virtual void visitedAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &) override {
        writer_.decIndent();
    }

    virtual void visitingNamedTemplateExprStmt(NamedTemplateExprStmt &templ) override {
        writer_.writeln("NamedTemplateExprStmt: " + templ.name().text());
        writer_.incIndent();
    }

    virtual void visitedNamedTemplateExprStmt(NamedTemplateExprStmt &) override {
        writer_.decIndent();
    }

    virtual void visitingTemplateExpansionExprStmt(TemplateExpansionExprStmt &expansion) override {
        writer_.writeln("TemplateExpansionExprStmt: " + expansion.templateName().qualifedName());
        writer_.incIndent();
    }

    virtual void visitedTemplateExpansionExprStmt(TemplateExpansionExprStmt &) override {
        writer_.decIndent();
    }

    void visitingModule(Module &module) override {
        writer_.writeln("Module: " + module.name());
        writer_.incIndent();
    }

    void visitedModule(Module &) override {
        writer_.decIndent();
    }
};

void prettyPrint(AstNode &module) {
    std::cerr<< "AST:\n";
    PrettyPrinterVisitor visitor{ std::cerr };
    module.accept(visitor);
    std::cerr << "\n";
}

}}}