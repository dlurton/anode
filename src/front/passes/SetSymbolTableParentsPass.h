
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
        if(!module.scope().parent()) {
            module.scope().setParent(world_.globalScope());
        }
    }

    void visitingFuncDefStmt(ast::FuncDefStmt &funcDeclStmt) override {
        if(!funcDeclStmt.parameterScope().parent()) {
            funcDeclStmt.parameterScope().setParent(topScope());
        }
        ScopeFollowingAstVisitor::visitingFuncDefStmt(funcDeclStmt);
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        if(!expansion.templateParameterScope().parent()) {
            expansion.templateParameterScope().setParent(topScope());
        }
        ScopeFollowingAstVisitor::visitingTemplateExpansionExprStmt(expansion);
    }

    void visitingCompoundExpr(ast::CompoundExpr &expr) override {
        //The first entry on the stack would be the global scope which has no parent
        if(scopeDepth() && !expr.scope().parent()) {
            expr.scope().setParent(topScope());
        }
        ScopeFollowingAstVisitor::visitingCompoundExpr(expr);
    }
};


}}}