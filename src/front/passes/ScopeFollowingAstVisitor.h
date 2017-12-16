
#pragma once
#include "ErrorContextAstVisitor.h"

namespace anode { namespace front  { namespace passes {



class ScopeFollowingAstVisitor : public ErrorContextAstVisitor {

    //We use this only as a stack but it has to be a deque so we can iterate over its contents.
    gc_ref_deque<scope::SymbolTable> symbolTableStack_;

protected:

    /** This is the topmost scope in the scope stack. */
    scope::SymbolTable &topScope() {
        ASSERT(symbolTableStack_.size());
        return symbolTableStack_.back().get();
    }

    /** This is the current scope from which all variables should be looked up.
     * This member function varies from {@ref topScope()} in that if the current scope a set of template expansions,
     * then it returns the top scope's parent scope.*/
    scope::SymbolTable &currentScope() {

        //A template expansion within a template will cause multiple chained template TemplateParameter scopes
        scope::SymbolTable *current = &topScope();

        while(current) {
            if (current->storageKind() != scope::StorageKind::TemplateParameter) {
                return *current;
            }
            current = current->parent();
        }
        ASSERT_FAIL("Looks like your top-most scope is a TemplateParameter scope.  Did you forget to set its parent?")
    }

    size_t scopeDepth() { return symbolTableStack_.size(); }


    ScopeFollowingAstVisitor(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

public:
    void pushScope(scope::SymbolTable &st) {
        symbolTableStack_.push_back(st);
    }

    void visitingFuncDefStmt(ast::FuncDefStmt &funcDeclStmt) override {
        symbolTableStack_.emplace_back(funcDeclStmt.parameterScope());
    }

    void visitedFuncDeclStmt(ast::FuncDefStmt &funcDeclStmt) override {
        ASSERT(&funcDeclStmt.parameterScope() == &symbolTableStack_.back().get())
        symbolTableStack_.pop_back();
    }

    void visitingNamespaceExpr(ast::NamespaceExpr &namespaceExpr) override {
        scope::SymbolTable *current = &currentScope();

        ast::MultiPartIdentifier::part_iterator end = namespaceExpr.name().end();
        for(ast::MultiPartIdentifier::part_iterator itr = namespaceExpr.name().begin(); itr != end; ++itr) {
            const ast::Identifier &nsName = (*itr);
            current = descendIntoNamespace(current, nsName);
            symbolTableStack_.emplace_back(*current);
        }

        namespaceExpr.setScope(*current);
    }

    scope::SymbolTable *descendIntoNamespace(scope::SymbolTable *current, const ast::Identifier &nsName) {
        ASSERT(current);
        auto found = current->findSymbolInCurrentScope(nsName.text());
        //No symbol matching the current part was found... create a new namespace in the current scope and descend into it.
        if(found == nullptr) {

            scope::SymbolTable &newScope = *new scope::SymbolTable(scope::StorageKind::Global, nsName.text());
            auto &newNs = *new scope::NamespaceSymbol(newScope);
            newNs.symbolTable().setParent(*current);
            current->addSymbol(newNs);
            current = &newNs.symbolTable();
        }
            //A symbol was found and it is a previously created namespace -- descend into it
        else if(auto nsSymbol = dynamic_cast<scope::NamespaceSymbol*>(found)) {
            current = &nsSymbol->symbolTable();
        }
            //A symbol was found and it isn't a previously created namespace.
        else {
            ASSERT_FAIL("Hrm... this code should be unreacahble.")
        }
        return current;
    }

    void visitedNamespaceExpr(ast::NamespaceExpr &namespaceExpr) override {
        ASSERT(&namespaceExpr.scope() == &symbolTableStack_.back().get());
        for(int i = 0; i < namespaceExpr.name().size(); ++i) {
            symbolTableStack_.pop_back();
        }
    }

    void visitingCompoundExpr(ast::CompoundExpr &compoundExpr) override {
        symbolTableStack_.emplace_back(compoundExpr.scope());
    }

    void visitedCompoundExpr(ast::CompoundExpr &compoundExpr) override {
        ASSERT(&compoundExpr.scope() == &symbolTableStack_.back().get());
        symbolTableStack_.pop_back();
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);
        symbolTableStack_.emplace_back(expansion.templateParameterScope());
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        ASSERT(&expansion.templateParameterScope() == &symbolTableStack_.back().get());
        symbolTableStack_.pop_back();
        ErrorContextAstVisitor::visitedTemplateExpansionExprStmt(expansion);
    }
};

}}}