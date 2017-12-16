
#pragma once

#include "ScopeFollowingAstVisitor.h"

namespace anode { namespace front  { namespace passes {

/** Sets each SymbolTable's parent scope.
 */
class SetSymbolTableParentsPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
public:
    SetSymbolTableParentsPass(error::ErrorStream &errorStream, ast::AnodeWorld &world_)
        : ScopeFollowingAstVisitor(errorStream), world_(world_) { }

    void visitingModule(ast::Module &module) override {
        module.scope().setParent(world_.globalScope());
    }
    void visitingFuncDefStmt(ast::FuncDefStmt &funcDeclStmt) override {
        funcDeclStmt.parameterScope().setParent(topScope());
        ScopeFollowingAstVisitor::visitingFuncDefStmt(funcDeclStmt);
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        //That currentScope() instead of topScope() is important because the scopes of expanded templates always need to
        //be parented to the outermost inner most scope not marked with scope::StorageKind::TemplateParameter.
        expansion.templateParameterScope().setParent(topScope());
        ScopeFollowingAstVisitor::visitingTemplateExpansionExprStmt(expansion);
    }

    void visitingCompoundExpr(ast::CompoundExpr &expr) override {
        //The first entry on the stack would be the global scope which has no parent
        if(scopeDepth()) {
            expr.scope().setParent(topScope());
        }
        ScopeFollowingAstVisitor::visitingCompoundExpr(expr);
    }
};


}}}