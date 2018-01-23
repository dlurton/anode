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

    void visitingCompoundExpr(CompoundExprStmt &expr) override {
        writer_.write(string::format("CompoundExprStmt: (%s) ", expr.scope().name().c_str()));
        writeScopeVariables(expr.scope());
        writer_.writeln();
        writer_.incIndent();
    }

    void visitedCompoundExpr(CompoundExprStmt &) override {
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

    void beforeVisit(BinaryExprStmt &expr) override {
        writer_.writeln("BinaryExprStmt: " + to_string(expr.operation()));
        writer_.incIndent();
    }

    void visitedBinaryExpr(BinaryExprStmt &) override {
        writer_.decIndent();
    }

    void visitingUnaryExpr(UnaryExprStmt &expr) override {
        writer_.writeln("UnaryExprStmt: " + to_string(expr.operation()) + ", " + expr.exprType().nameForDisplay());
        writer_.incIndent();
    }

    void visitedUnaryExpr(UnaryExprStmt &) override {
        writer_.decIndent();
    }

    void visitingDotExpr(DotExprStmt &expr) override {
        writer_.writeln("DotExprStmt: " + expr.memberName().text());
        writer_.incIndent();
    }

    void visitedDotExpr(DotExprStmt &) override {
        writer_.decIndent();
    }

    void visitLiteralBoolExpr(LiteralBoolExprStmt &expr) override {
        writer_.writeln("LiteralBoolExprStmt: %s", expr.value() ? "true" : "false");
    }

    void visitLiteralInt32Expr(LiteralInt32ExprStmt &expr) override {
        writer_.writeln("LiteralInt32ExprStmt: " + std::to_string(expr.value()));
    }

    void visitLiteralFloatExpr(LiteralFloatExprStmt &expr) override {
        writer_.writeln("LiteralFloat: " + std::to_string(expr.value()));
    }

    void beforeVisit(VariableDeclExpr &expr) override {
        writer_.writeln("VariableDeclExpr: (%s:%s)", expr.name().qualifedName().c_str(),
                        expr.typeRef().name().qualifedName().c_str());
        writer_.incIndent();
    }

    void visitedVariableDeclExpr(VariableDeclExpr &) override {
        writer_.decIndent();

    }
    void visitVariableRefExpr(VariableRefExprStmt &expr) override {
        writer_.writeln("VariableRefExprStmt: " + expr.name().qualifedName());
    }

    void beforeVisit(IfExprStmt &) override {
        writer_.writeln("IfExprStmt:");
        writer_.incIndent();
    }

    void visitedIfExpr(IfExprStmt &) override {
        writer_.decIndent();
    }

    void beforeVisit(WhileExprStmt &) override {
        writer_.writeln("WhileExprStmt:");
        writer_.incIndent();
    }

    void visitedWhileExpr(WhileExprStmt &) override {
        writer_.decIndent();
    }


    void beforeVisit(AssertExprStmt &) override {
        writer_.writeln("AssertExprStmt:");
        writer_.incIndent();
    }

    void visitedAssertExprStmt(AssertExprStmt &) override {
        writer_.decIndent();
    }

    void visitingCastExprStmt(CastExprStmtStmt &expr) override {
        writer_.writeln("CastExprStmtStmt(%s):",
                        expr.castKind() == CastKind::Implicit ? "implicit" : "explicit");

        writer_.incIndent();
    }

    void visitingNewExpr(NewExprStmt &expr) override {
        writer_.writeln("NewExprStmt(%s):", expr.typeRef().name().qualifedName().c_str());

        writer_.incIndent();
    }

    void visitedCastExprStmt(CastExprStmtStmt &) override {
        writer_.decIndent();
    }

    void beforeVisit(ParameterDef &pd) override {
        writer_.write("ParameterDef: ");
        writer_.write(pd.name().text());
        writer_.write(':');
        writer_.writeln(pd.typeRef().name().qualifedName().c_str());
    }

    void beforeVisit(FuncDefExprStmt &func) override {
        writer_.writeln("FuncDefExprStmt: " + func.name().text());
//                auto parameters = func.parameters();
//                for(auto p : parameters) {
//                    if(p != parameters.front()) {
//                        writer_.write(", ");
//                    }
//                    writer_.write();
//                }
        writer_.incIndent();
    }

    void beforeVisit(FuncCallExprStmt &) override {
        writer_.writeln("FuncCallExprStmt:");
        writer_.incIndent();
    }

    void visitedFuncCallExprStmt(FuncCallExprStmt &) override {
        writer_.decIndent();
    }

    void visitedFuncDeclStmt(FuncDefExprStmt &) override {
        writer_.decIndent();
    }

    void beforeVisit(CompleteClassDefExprStmt &cd) override {
        writer_.writeln("ClassDefinition: " + cd.name().text());
        writer_.incIndent();
    }

    void visitedCompleteClassDefExprStmt(CompleteClassDefExprStmt &) override {
        writer_.decIndent();
    }

    void beforeVisit(GenericClassDefExprStmt &cd) override {
        writer_.writeln("GenericClassDefExprStmt: " + cd.name().text());
        writer_.incIndent();
    }

    void visitedGenericClassDefExprStmt(GenericClassDefExprStmt &) override {
        writer_.decIndent();
    }

    void visitingExpressionList(ExpressionListStmt &) override {
        writer_.writeln("ExpressionListStmt");
        writer_.incIndent();
    }
    void visitedExpressionList(ExpressionListStmt &) override {
        writer_.decIndent();
    }

    void visitingAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &exprStmt) override {
        writer_.writeln("AnonymousTemplateExprStmt");
        writer_.incIndent();
        //children of anonymous templates are not normally visited...
        exprStmt.body().acceptVisitor(*this);
    }

    void visitedAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &) override {
        writer_.decIndent();
    }

    void visitingNamedTemplateExprStmt(NamedTemplateExprStmt &templ) override {
        writer_.writeln("NamedTemplateExprStmt: " + templ.name().text());
        writer_.incIndent();
        templ.body().acceptVisitor(*this);
    }

    void visitedNamedTemplateExprStmt(NamedTemplateExprStmt &) override {
        writer_.decIndent();
    }

    void visitingTemplateExpansionExprStmt(TemplateExpansionExprStmt &expansion) override {
        writer_.writeln("TemplateExpansionExprStmt: " + expansion.templateName().qualifedName());
        writer_.incIndent();

    }

    void visitedTemplateExpansionExprStmt(TemplateExpansionExprStmt &) override {
        writer_.decIndent();
    }

    void visitingNamespaceExpr(ast::NamespaceExprStmt &namespaceExpr) override {
        writer_.writeln("NamespaceExprStmt: " + namespaceExpr.name().qualifedName());
        writer_.incIndent();
    }

    void visitedNamespaceExpr(ast::NamespaceExprStmt &) override {
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
    module.acceptVisitor(visitor);
    std::cerr << "\n";
}

}}}