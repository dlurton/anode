
#pragma once

#include "ErrorContextAstVisitor.h"

namespace anode { namespace front  { namespace passes {


/**
 * Adds each CompleteClassDefinition instance and the template arguments used to expand it to the GenericClassDefinition
 * from which it originated so that it may later be resolved.
 */
class PopulateGenericTypesWithCompleteTypesPass : public ErrorContextAstVisitor {

    class PopulateGenericTypesSubPass : public ScopeFollowingAstVisitor {
        gc_ref_vector<type::Type> templateArguments_;
    public:
        PopulateGenericTypesSubPass(error::ErrorStream &errorStream, const gc_ref_vector<type::Type> &templateArgs)
            : ScopeFollowingAstVisitor(errorStream), templateArguments_(templateArgs) { }

        void visitingCompleteClassDefinition(ast::CompleteClassDefinition &cd) override {
            auto genericType = upcast<type::ClassType>(cd.definedType()).genericType();
            ASSERT(genericType);
            if(genericType->findExpandedClassType(templateArguments_)) {
                return;
            }
            genericType->addExpandedClass(templateArguments_, upcast<type::ClassType>(cd.definedType()));
        }
    };

public:
    explicit PopulateGenericTypesWithCompleteTypesPass(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);

        gc_ref_vector<type::Type> typeArguments;
        gc_ref_vector<scope::TypeSymbol> argumentSymbols = expansion.templateParameterScope().types();
        typeArguments.reserve(argumentSymbols.size());
        for(scope::TypeSymbol &argSymbol : argumentSymbols) {
            typeArguments.emplace_back(argSymbol.type());
        }

        PopulateGenericTypesSubPass pass{errorStream_, typeArguments};
        pass.pushScope(expansion.templateParameterScope());
        expansion.expandedTemplate()->accept(pass);
    }
};


}}}