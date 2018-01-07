
#pragma once

#include "ErrorContextAstVisitor.h"
#include "run_passes.h"

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
            if(genericType) {
                if (genericType->findExpandedClassType(templateArguments_)) {
                    return;
                }
                genericType->addExpandedClass(templateArguments_, upcast<type::ClassType>(cd.definedType()));
            }
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

//FIXME:  move to new file, pick a better name
class ExpandClassesWithinAnonymousTemplates : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
    ast::Module &module_;
public:
    ExpandClassesWithinAnonymousTemplates(error::ErrorStream &errorStream, ast::Module &module, ast::AnodeWorld &world)
        : ScopeFollowingAstVisitor(errorStream), world_{world}, module_{module} {

    }
    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef &typeRef) override {
        if(typeRef.hasTemplateArguments()) {

            //Look up type the generic type:
            auto maybeTypeSymbol = findQualifiedSymbol(topScope(), typeRef.name(), errorStream_);
            if(maybeTypeSymbol == nullptr) {
                //Error message handled by findQualifiedSymbol(...)
                return;
            }

            auto typeSymbol = dynamic_cast<scope::TypeSymbol*>(maybeTypeSymbol);
            if(!typeSymbol) {
                ASSERT_FAIL("TODO:  resolved symbol is not a type");
                return;
            }

            auto genericType = dynamic_cast<type::GenericType*>(typeSymbol->type().actualType());
            if(!genericType) {
                errorStream_.error(
                    error::ErrorKind::TypeIsNotGenericButIsReferencedWithGenericArgs,
                    typeRef.sourceSpan(),
                    "Type '%s' is not generic but is referenced with generic arguments",
                    typeRef.type().nameForDisplay().c_str());
                return;
            }

            auto &templateArgTypes = typeRef.templateArgsTypes();
            if(genericType->templateParameterCount() != (int)templateArgTypes.size()) {
                errorStream_.error(
                    error::ErrorKind::IncorrectNumberOfGenericArguments,
                    typeRef.sourceSpan(),
                    "Incorrect number of generic arguments for type '%s' - expected %d but found %d",
                    genericType->nameForDisplay().c_str(),
                    genericType->templateParameterCount(),
                    (int)templateArgTypes.size());
                return;
            }

            auto foundClassType = genericType->findExpandedClassType(templateArgTypes);
            if(foundClassType == nullptr) {
                auto &genericClass = world_.getGenericClassDefinition(genericType->astNodeId());

                //FIXME:  the term "template arguments" seems to be rather overloaded here, since "TemplateArgVector" means
                //something a bit different than typeRef.templateArg*s()
                //May need to refactor some things so that they store TemplateArgument instead of TypeResolutionDeferredTypeRef...

                //Constitute a proper array of TemplateArgument (which has name and typeRef)
                ast::TemplateArgVector argVector;
                argVector.reserve(templateArgTypes.size());
                auto argNames = genericClass.templateParameters();
                auto argTypeRefs = typeRef.templateArgTypeRefs();
                for(int i = 0; i < (int)templateArgTypes.size(); ++i) {
                    argVector.emplace_back(*new ast::TemplateArgument(argNames[i].get().name(), argTypeRefs[i]));
                }
                ast::TemplateExpansionContext context{ast::ExpansionKind::AnonymousTemplate, argVector};
                auto &completedClass = upcast<ast::ClassDefinition>(genericClass.deepCopyExpandTemplate(context));
                genericType->addExpandedClass(templateArgTypes, upcast<type::ClassType>(completedClass.definedType()));


                gc_ref_vector<ast::ExprStmt> compoundExprBody;
                compoundExprBody.emplace_back(completedClass);
                auto &compoundExpr = *new ast::CompoundExpr(
                    completedClass.sourceSpan(),
                    scope::StorageKind::TemplateParameter,
                    compoundExprBody,
                    typeSymbol->fullyQualifiedName() + scope::ScopeSeparator + "ImplicitExpansion");

                for(ast::TemplateArgument &templateArg : argVector) {
                    compoundExpr.scope().addSymbol(
                        *new scope::TypeSymbol(templateArg.parameterName().text(), templateArg.typeRef().type()));
                }

                module_.body().append(compoundExpr);
                auto passes = getPreTemplateExpansionPassses(world_, module_, errorStream_);
                runPasses(passes, compoundExpr, errorStream_, currentScope());
                if(errorStream_.errorCount() == 0) {
                    ResolveTypesPass pass{errorStream_};
                    completedClass.accept(pass);
                }
            }
        }
    }
};


}}}