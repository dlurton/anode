
#pragma once

#include "ScopeFollowingAstVisitor.h"
#include "symbol_search.h"

namespace anode { namespace front  { namespace passes {


class ResolveSymbolsPass : public ScopeFollowingAstVisitor {
    gc_unordered_set<scope::Symbol *> definedSymbols_;
public:
    explicit ResolveSymbolsPass(error::ErrorStream &errorStream_) : ScopeFollowingAstVisitor(errorStream_) {}

    void visitingVariableDeclExpr(ast::VariableDeclExpr &expr) override {
        ASSERT(expr.symbol() && "Symbol must be resolved before this point.");
        if (expr.symbol()->storageKind() == scope::StorageKind::Local) {
            definedSymbols_.emplace(expr.symbol());
        }
    }

    void visitVariableRefExpr(ast::VariableRefExpr &expr) override {
        if (expr.symbol()) return;

        scope::Symbol *found = findQualifiedSymbol(currentScope(), expr.name(), errorStream_);
        if (found) {

            if (!found->type().isClass() && !found->type().isFunction()) {
                if (found->storageKind() == scope::StorageKind::Local && definedSymbols_.count(found) == 0) {
                    errorStream_.error(
                        error::ErrorKind::VariableUsedBeforeDefinition, expr.sourceSpan(), "'%s' was used before its definition.",
                        expr.name().qualifedName().c_str());
                    return;
                }
            }

            expr.setSymbol(*found);
        }
    }
};

}}}