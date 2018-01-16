
#pragma once


#include "ErrorContextAstVisitor.h"
#include "run_passes.h"
#include "ResolveTypesPass.h"

namespace anode { namespace front  { namespace passes {


class ExpandGenericTypeReferencesPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
    ast::Module &module_;

public:
    ExpandGenericTypeReferencesPass(error::ErrorStream &errorStream, ast::Module &module, ast::AnodeWorld &world)
        : ScopeFollowingAstVisitor(errorStream), world_{world}, module_{module} {

    }

    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef &typeRef) override {

        if(typeRef.hasTemplateArguments()) {
            auto genericType = dynamic_cast<type::GenericType*>(typeRef.type().actualType());
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
                auto expandedTemplateWrapper = new ast::CompoundExpr(
                    completedClass.sourceSpan(),
                    scope::StorageKind::TemplateParameter,
                    compoundExprBody,
                    genericClass.symbol()->fullyQualifiedName() + scope::ScopeSeparator + "ImplicitExpansion");

                for(ast::TemplateArgument &templateArg : argVector) {
                    expandedTemplateWrapper->scope().addSymbol(
                        *new scope::TypeSymbol(templateArg.parameterName().text(), templateArg.typeRef().type()));
                }
                ast::ExprStmt *expandedTemplate = expandedTemplateWrapper;

                expandedTemplateWrapper->scope().setParent(genericClass.symbol()->symbolTable());

                module_.body().append(*expandedTemplate);
                auto passes = getPreTemplateExpansionPassses(world_, module_, errorStream_);
                runPasses(passes, *expandedTemplate, errorStream_, expandedTemplateWrapper->scope());
                if(errorStream_.errorCount() == 0) {
                    ResolveTypesPass pass{errorStream_};
                    completedClass.accept(pass);
                }
            }
        }
    }
};

}}}