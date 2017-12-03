
#pragma once

#include "ErrorContextAstVisitor.h"
#include "symbol_search.h"
#include "run_passes.h"

namespace anode { namespace front  { namespace passes {


/** Expands explicit template expansions. */
class TemplateExpanderPass : public ErrorContextAstVisitor {
    ast::AnodeWorld &world_;
    ast::Module *module_ = nullptr;
    bool visitingExpansion_ = false;
public:

    TemplateExpanderPass(error::ErrorStream &errorStream, ast::AnodeWorld &world_)
        : ErrorContextAstVisitor(errorStream), world_(world_) { }

    bool shouldVisitChildren() override {
        return !visitingExpansion_;
    }

    void visitingModule(ast::Module &module) override {
        module_ = &module;
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);
        visitingExpansion_ = true;
        if(expansion.expandedTemplate() != nullptr) {
            return;
        }
        ASSERT(expansion.templateParameterScope().parent());
        scope::Symbol *foundSymbol = findQualifiedSymbol(
            *expansion.templateParameterScope().parent(),
            expansion.templateName(), errorStream_);

        if(!foundSymbol) {
            //Error message handled by findQualifiedSymbol(...)
            return;
        }

        auto templateSymbol = dynamic_cast<scope::TemplateSymbol*>(foundSymbol);
        if(templateSymbol == nullptr) {
            errorStream_.error(
                error::ErrorKind::SymbolIsNotATemplate,
                expansion.templateName().span(),
                "Symbol '%s' is not a template",
                expansion.templateName().qualifedName().c_str());
            return;
        }
        auto &templ = world_.getTemplate(templateSymbol->astNodeId());
        expansion.setTempl(templ);

        if(world_.isExpanding(templ)) {
            errorStream_.error(
                error::ErrorKind::CircularTemplateReference,
                expansion.sourceSpan(),
                "Cannot expand template '%s' -- circular template expansion detected",
                expansion.templateName().front().text().c_str());
        }
        world_.addExpandingTemplate(templ);

        //For each template argument
        //Create create a Symbol of name of the corresponding parameter referring to the TypeRef.  (AliasSymbol?)
        gc_ref_vector<ast::TemplateParameter> tParams = templ.parameters();
        gc_ref_vector<ast::TypeRef> tArgs = expansion.typeArguments();

        if(tParams.size() != tArgs.size()) {
            const char *wasWere = tArgs.size() == 1 ? "was" : "were";
            errorStream_.error(
                error::ErrorKind::IncorrectNumberOfTemplateArguments,
                expansion.sourceSpan(),
                "Incorrect number of template arguments - expected %d but %d %s specified",
                tParams.size(),
                tArgs.size(),
                wasWere);
            return;
        }

        //TODO:  refactor TemplateExpansionExprStmt to store this as a field.
        gc_ref_vector<ast::TemplateArgument> templateArgs;

        for(unsigned int i = 0; i < tParams.size(); ++i) {
            expansion.templateParameterScope().addSymbol(*new scope::TypeSymbol(tParams[i].get().name().text(), tArgs[i].get().type()));
            templateArgs.emplace_back(*new ast::TemplateArgument(tParams[i].get().name().text(), tArgs[i].get()));
        }

        expansion.setExpandedTemplate(&templ.body().deepCopyExpandTemplate(templateArgs));

        auto visitors = getPreTemplateExpansionPassses(world_, errorStream_);
        runPasses(visitors, *expansion.expandedTemplate(), errorStream_, expansion.templateParameterScope());
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        visitingExpansion_ = false;
        world_.removeExpandingTemplate(expansion.templ());
        ErrorContextAstVisitor::visitedTemplateExpansionExprStmt(expansion);

    }
};

}}}