
#pragma once


#include "ErrorContextAstVisitor.h"
#include "run_passes.h"
#include "ResolveTypesPass.h"
#include "front/visualize.h"

namespace anode { namespace front  { namespace passes {

class LineageCalculatingPass : public ast::AstVisitor {
private:
    gc_deque<ast::AstNode*> nodeStack_;
public:
    virtual void beforeAccept(ast::AstNode &node) override {
        if(nodeStack_.size() > 0) {
            node.setParent(*nodeStack_.back());
        }
        nodeStack_.push_back(&node);
    }
    virtual void afterAccept(ast::AstNode &node) override {
        ASSERT(nodeStack_.back() == &node);
        nodeStack_.pop_back();
    }


    void visitingNamedTemplateExprStmt(ast::NamedTemplateExprStmt &templ) override {
        //body not normally visited
        templ.body().acceptVisitor(*this);
    }

    void visitingGenericClassDefinition(ast::GenericClassDefinition &genericClassDefinition) override {
        //body not normally visited
        genericClassDefinition.body();
    }

    void visitingAnonymousTemplateExprStmt(ast::AnonymousTemplateExprStmt &anonymousTemplateExprStmt) override {
        //Body of anonymous template not normally visited...
        anonymousTemplateExprStmt.body().acceptVisitor(*this);
    }
};

class ExpandGenericTypeReferencesPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
    ast::Module &module_;

public:
    ExpandGenericTypeReferencesPass(error::ErrorStream &errorStream, ast::Module &module, ast::AnodeWorld &world)
        : ScopeFollowingAstVisitor(errorStream), world_{world}, module_{module} {

    }

    void visitingModule(ast::Module &module) override {
        LineageCalculatingPass lineageCalculatingPass;
        module.acceptVisitor(lineageCalculatingPass);
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

                //TODO: set expandedTemplateWrapper->scope() to an instance of DelegateSymbolTable
                expandedTemplateWrapper->scope().setParent(genericClass.symbol()->symbolTable());

                module_.body().append(*expandedTemplate);
                visualize::prettyPrint(module_);

                auto passes = getPreTemplateExpansionPassses(world_, module_, errorStream_);
                runPasses(passes, *expandedTemplate, errorStream_, expandedTemplateWrapper->scope());
                if(errorStream_.errorCount() == 0) {
                    ResolveTypesPass pass{errorStream_};
                    completedClass.acceptVisitor(pass);
                }
            }
        }
    }
};

}}}