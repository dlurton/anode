
#pragma once

namespace anode { namespace front { namespace passes {

class PopulateSymbolTablesPass : public ScopeFollowingAstVisitor {
public:
    explicit PopulateSymbolTablesPass(error::ErrorStream &errorStream)
        : ScopeFollowingAstVisitor(errorStream) {  }

    void symbolPreviouslyDefinedError(const ast::Identifier &identifier) {
        errorStream_.error(
            error::ErrorKind::SymbolAlreadyDefinedInScope,
            identifier.span(),
            "Symbol '%s' was previously defined in the current scope",
            identifier.text().c_str());
    }

    void beforeVisit(ast::CompleteClassDefExprStmt &cd) override {

        // Classes that are defined within expanded templates do not get their own symbols
        // Only the generic version of them do. During symbol resolution, the symbol of the GenericType is
        // resolved and the resolved GenericType is used to determine the Type of the expanded class.
        if (!cd.hasTemplateArguments()) {
            type::Type &definedType = cd.definedType();

            if (auto definedClassType = dynamic_cast<type::ClassType *>(&definedType)) {
                if (definedClassType->genericType() != nullptr) {
                    ScopeFollowingAstVisitor::beforeVisit(cd);
                    return;
                }
            }

            if(currentScope().findSymbolInCurrentScope(cd.name().text())) {
                symbolPreviouslyDefinedError(cd.name());
            } else {
                auto &&classSymbol = *new scope::TypeSymbol(definedType);
                currentScope().addSymbol(classSymbol);
            }
        }
        ScopeFollowingAstVisitor::beforeVisit(cd);
    }

    void visitingFuncDefExprStmt(ast::FuncDefExprStmt &funcDeclStmt) override {
        if(currentScope().findSymbolInCurrentScope(funcDeclStmt.name().text())) {
            symbolPreviouslyDefinedError(funcDeclStmt.name());
        } else {
            scope::FunctionSymbol &funcSymbol = *new scope::FunctionSymbol(funcDeclStmt.name().text(), &funcDeclStmt.functionType());

            currentScope().addSymbol(funcSymbol);
            funcDeclStmt.setSymbol(funcSymbol);

            for (auto p : funcDeclStmt.parameters()) {
                scope::VariableSymbol &symbol = *new scope::VariableSymbol(p.get().name().text(), p.get().type());
                if (funcDeclStmt.parameterScope().findSymbolInCurrentScope(p.get().name().text())) {
                    errorStream_.error(
                        error::ErrorKind::SymbolAlreadyDefinedInScope,
                        p.get().sourceSpan(),
                        "Duplicate parameter name '%s'",
                        p.get().name().text().c_str());
                } else {
                    funcDeclStmt.parameterScope().addSymbol(symbol);
                    p.get().setSymbol(symbol);
                }
            }
        }
        ScopeFollowingAstVisitor::visitingFuncDefExprStmt(funcDeclStmt);
    }

    void beforeVisit(ast::VariableDeclExpr &expr) override {
        ASSERT(expr.name().size() == 1 && "TODO:  semantic error when variable declarations have more than 1 part or refactor VariableDeclExpr and VariableRefExprStmt.");

        if(currentScope().findSymbolInCurrentScope(expr.name().front().text())) {
            symbolPreviouslyDefinedError(expr.name().front());
        } else {
            auto &&symbol = *new scope::VariableSymbol(expr.name().front().text(), expr.typeRef().type());
            currentScope().addSymbol(symbol);
            expr.setSymbol(symbol);
        }
    }

    void visitingAnonymousTemplateExprStmt(ast::AnonymousTemplateExprStmt &templ) override {
        auto cd = upcast<ast::GenericClassDefExprStmt>(&templ.body());
        if (currentScope().findSymbolInCurrentScope(cd->name().text())) {
            symbolPreviouslyDefinedError(cd->name());
        } else {
            auto &&symbol = *new scope::TypeSymbol(cd->name().text(), cd->definedType());
            currentScope().addSymbol(symbol);
            cd->setSymbol(symbol);
        }
    }

    void visitingNamedTemplateExprStmt(ast::NamedTemplateExprStmt &templ) override {
        if (currentScope().findSymbolInCurrentScope(templ.name().text())) {
            symbolPreviouslyDefinedError(templ.name());
        } else {
            auto &&symbol = *new scope::TemplateSymbol(templ.name().text(), templ.nodeId());
            currentScope().addSymbol(symbol);
        }
    }
};

}}}