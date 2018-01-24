#pragma once

#include "anode.h"
#include "ast.h"

namespace anode { namespace front { namespace ast {

/**
 * #1 rule when inheriting from this class:
 *  Never invoke the visit*() member functions directly.
 *  They should only ever be invoked by the public visit*() functions.
 *  This is because an inheritor may wish to provide special handling in one of those functions and invoking the
 *  protected visit*() will bypass that.
 */
class AstRewriter : public gc {
public:
    virtual AstNode *visit(AstNode *node) {
        if(node == nullptr) return nullptr;
        switch(node->astNodeKind()) {
            case AstNodeKind::Module:
                return visitModule(downcast<Module>(node));
            case AstNodeKind::ParameterDef:
                return visitParameterDef(downcast<ParameterDef>(node));
            case AstNodeKind::ExprStmt:
                return visitExprStmt(downcast<ExprStmt>(node));
            case AstNodeKind::TemplateParameter:
                return visitTemplateParameter(downcast<TemplateParameter>(node));
            case AstNodeKind::TypeRef:
                return visitTypeRef(downcast<TypeRef>(node));
    
            default:
                ASSERT_FAIL("Unhandled AstNodeKind");
        }
    }
    
    virtual TypeRef *visitTypeRef(TypeRef *typeRef) {
        if(typeRef == nullptr) return nullptr;
        switch(typeRef->typeRefKind()) {
            case TypeRefKind::Known:
                return visitKnownTypeRef(downcast<KnownTypeRef>(typeRef));
            case TypeRefKind::ResolutionDeferred:
                return visitResolutionDeferredTypeRef(downcast<ResolutionDeferredTypeRef>(typeRef));
            default:
                ASSERT_FAIL("Unhandled TypeRefKind")
        }
    }
    
    virtual ExprStmt *visitExprStmt(ExprStmt *exprStmt) {
        if(exprStmt == nullptr) return nullptr;
        switch(exprStmt->exprStmtKind()) {
            case ExprStmtKind::Compound:
                return visitCompoundExprStmt(downcast<CompoundExprStmt>(exprStmt));
            case ExprStmtKind::ExpressionList:
                return visitExpressionListExprStmt(downcast<ExpressionListExprStmt>(exprStmt));
            case ExprStmtKind::FuncDef:
                return visitFuncDefExprStmt(downcast<FuncDefExprStmt>(exprStmt));
            case ExprStmtKind::FuncCall:
                return visitFuncCallExprStmt(downcast<FuncCallExprStmt>(exprStmt));
            case ExprStmtKind::GenericClassDef:
                return visitGenericClassDefExprStmt(downcast<GenericClassDefExprStmt>(exprStmt));
            case ExprStmtKind::CompleteClassDef:
                return visitCompleteClassDefExprStmt(downcast<CompleteClassDefExprStmt>(exprStmt));
            case ExprStmtKind::MethodRef:
                return visitMethodRefExprStmt(downcast<MethodRefExprStmt>(exprStmt));
            case ExprStmtKind::If:
                return visitIfExprStmt(downcast<IfExprStmt>(exprStmt));
            case ExprStmtKind::While:
                return visitWhileExprStmt(downcast<WhileExprStmt>(exprStmt));
            case ExprStmtKind::LiteralBool:
                return visitLiteralBoolExprStmt(downcast<LiteralBoolExprStmt>(exprStmt));
            case ExprStmtKind::LiteralInt32:
                return visitLiteralInt32ExprStmt(downcast<LiteralInt32ExprStmt>(exprStmt));
            case ExprStmtKind::LiteralFloat:
                return visitLiteralFloatExprStmt(downcast<LiteralFloatExprStmt>(exprStmt));
            case ExprStmtKind::Unary:
                return visitUnaryExprStmt(downcast<UnaryExprStmt>(exprStmt));
            case ExprStmtKind::Binary:
                return visitBinaryExprStmt(downcast<BinaryExprStmt>(exprStmt));
            case ExprStmtKind::Dot:
                return visitDotExprStmt(downcast<DotExprStmt>(exprStmt));
            case ExprStmtKind::VariableDecl:
                return visitVariableDeclExprStmt(downcast<VariableDeclExprStmt>(exprStmt));
            case ExprStmtKind::VariableRef:
                return visitVariableRefExprStmt(downcast<VariableRefExprStmt>(exprStmt));
            case ExprStmtKind::Cast:
                return visitCastExprStmt(downcast<CastExprStmt>(exprStmt));
            case ExprStmtKind::New:
                return visitNewExprStmt(downcast<NewExprStmt>(exprStmt));
            case ExprStmtKind::Assert:
                return visitAssertExprStmt(downcast<AssertExprStmt>(exprStmt));
            case ExprStmtKind::Namespace:
                return visitNamespaceExprStmt(downcast<NamespaceExprStmt>(exprStmt));
            case ExprStmtKind::AnonymousTemplate:
                return visitAnonymousTemplateExprStmt(downcast<AnonymousTemplateExprStmt>(exprStmt));
            case ExprStmtKind::NamedTemplate:
                return visitNamedTemplateExprStmt(downcast<NamedTemplateExprStmt>(exprStmt));
            case ExprStmtKind::TemplateExpansion:
                return visitTemplateExpansionExprStmt(downcast<TemplateExpansionExprStmt>(exprStmt));
            default:
                ASSERT_FAIL("Unhandled ExprStmtKind")
        }
    }

protected:
    
    virtual Identifier visitIdentifier(const Identifier &identifier) {
        return identifier;
    }
    
    virtual MultiPartIdentifier visitMultiPartIdentifier(const MultiPartIdentifier &multiPartIdentifier) {
        return multiPartIdentifier;
    }
    
    virtual ParameterDef *visitParameterDef(ParameterDef *node) {
        return new ParameterDef(
            node->sourceSpan(),
            visitIdentifier(node->name()),
            *visitTypeRef(&node->typeRef()));
    }
    
    virtual gc_ref_vector<ParameterDef> visitTemplateParameters(const gc_ref_vector<ParameterDef> &parametersToVisit) {
        gc_ref_vector<ParameterDef> visitedParameters;
        visitedParameters.reserve(parametersToVisit.size());
        for(ParameterDef &p : parametersToVisit) {
            visitedParameters.emplace_back(*visitParameterDef(&p));
        }
        return visitedParameters;
    }
    virtual ExprStmt* visitFuncDefExprStmt(FuncDefExprStmt *node) {
       
        return new FuncDefExprStmt(
            node->sourceSpan(),
            node->name(),
            *visitTypeRef(&node->returnTypeRef()),
            visitTemplateParameters(node->parameters()),
            *visitExprStmt(&node->body()));
    }
    
    virtual gc_ref_vector<ExprStmt> visitExprStmts(const gc_ref_vector<ExprStmt> stmtsToVisit) {
        gc_ref_vector<ExprStmt> visitedStmts;
        visitedStmts.reserve(stmtsToVisit.size());
        for(ExprStmt &a : stmtsToVisit) {
            visitedStmts.emplace_back(*visitExprStmt(&a));
        }
        
        return visitedStmts;
    }
    
    virtual ExprStmt* visitFuncCallExprStmt(FuncCallExprStmt *node) {
        
        return new FuncCallExprStmt(
            node->sourceSpan(),
            visitExprStmt(node->instanceExpr()),
            node->openParenSpan(),
            *visitExprStmt(&node->funcExpr()),
            visitExprStmts(node->arguments()));
    }
    
    
    virtual ExprStmt* visitGenericClassDefExprStmt(GenericClassDefExprStmt *node) {
        return new GenericClassDefExprStmt(
            node->sourceSpan(),
            visitIdentifier(node->name()),
            visitTemplateParameters(node->templateParameters()),
            *downcast<CompoundExprStmt>(visitExprStmt(&node->body())));
    }
    
    virtual gc_ref_vector<TemplateArgument> visitTemplateArguments(const gc_ref_vector<TemplateArgument> &argsToVisit) {
        gc_ref_vector<TemplateArgument> visitedArgs;
        visitedArgs.reserve(argsToVisit.size());
        for(auto &arg : argsToVisit) {
            visitedArgs.emplace_back(*visitTemplateArgument(&arg.get()));
        }
        return visitedArgs;
    }
    
    virtual TemplateArgument* visitTemplateArgument(TemplateArgument *templateArgument) {
        return new TemplateArgument(
            templateArgument->parameterName(),
            *visitTypeRef(&templateArgument->typeRef()));
    }
    
    virtual ExprStmt* visitCompleteClassDefExprStmt(CompleteClassDefExprStmt *node) {
        return new CompleteClassDefExprStmt(
            node->sourceSpan(),
            visitIdentifier(node->name()),
            visitTemplateArguments(node->templateArguments()),
            downcast<CompoundExprStmt>(*visitExprStmt(&node->body())));
    
    }
    
    virtual ExprStmt* visitAssertExprStmt(AssertExprStmt *node) {
        return new AssertExprStmt(
            node->sourceSpan(),
            *visitExprStmt(&node->condition()));
    }
    
    virtual ExprStmt* visitVariableDeclExprStmt(VariableDeclExprStmt *node) {
        return new VariableDeclExprStmt(
            node->sourceSpan(),
            visitMultiPartIdentifier(node->name()),
            *visitTypeRef(&node->typeRef()),
            node->variableAccess());
    }
    
    virtual ExprStmt* visitIfExprStmt(IfExprStmt *node) {
        return new IfExprStmt(
            node->sourceSpan(),
            *visitExprStmt(&node->condition()),
            *visitExprStmt(&node->thenExpr()),
            visitExprStmt(node->elseExpr()));
    }
    
    virtual ExprStmt* visitWhileExprStmt(WhileExprStmt *node) {
        return new WhileExprStmt(
            node->sourceSpan(),
            *visitExprStmt(&node->condition()),
            *visitExprStmt(&node->body()));
    }
    
    virtual ExprStmt* visitBinaryExprStmt(BinaryExprStmt *node) {
        return new BinaryExprStmt(
            node->sourceSpan(),
            *visitExprStmt(&node->lValue()),
            node->operation(),
            node->operatorSpan(),
            *visitExprStmt(&node->rValue()));
    }
    
    
    virtual ExprStmt* visitUnaryExprStmt(UnaryExprStmt *node) {
        return new UnaryExprStmt(
            node->sourceSpan(),
            *visitExprStmt(&node->valueExpr()),
            node->operation(),
            node->operatorSpan());
    }
    
    virtual ExprStmt* visitLiteralBoolExprStmt(LiteralBoolExprStmt *node) {
        return node;
    }
    
    virtual ExprStmt* visitLiteralInt32ExprStmt(LiteralInt32ExprStmt *node) {
        return node;
    }
    
    virtual ExprStmt* visitLiteralFloatExprStmt(LiteralFloatExprStmt *node) {
        return node;
    }
    
    virtual ExprStmt* visitVariableRefExprStmt(VariableRefExprStmt *node) {
        return new VariableRefExprStmt(
            node->sourceSpan(),
            visitMultiPartIdentifier(node->name()),
            node->variableAccess());
    }
    
    virtual ExprStmt* visitMethodRefExprStmt(MethodRefExprStmt *node) {
        return new MethodRefExprStmt(visitIdentifier(node->name()));
    }
    
    virtual ExprStmt* visitCastExprStmt(CastExprStmt *node) {
        return new CastExprStmt(
            node->sourceSpan(),
            *visitTypeRef(&node->toTypeRef()),
            *visitExprStmt(&node->valueExpr()),
            node->castKind());
    
    }
    
    virtual ExprStmt* visitNewExprStmt(NewExprStmt *node) {
        return new NewExprStmt(
            node->sourceSpan(),
            *visitTypeRef(&node->typeRef()));
    }
    
    virtual ExprStmt* visitDotExprStmt(DotExprStmt *node) {
        return new DotExprStmt(
            node->sourceSpan(),
            node->dotSourceSpan(),
            *visitExprStmt(&node->lValue()),
            visitIdentifier(node->memberName()));
    }
    
    virtual ExprStmt* visitCompoundExprStmt(CompoundExprStmt *node) {
        return new CompoundExprStmt(
            node->sourceSpan(),
            node->scope().storageKind(),
            visitExprStmts(node->expressions()),
            node->scope().name());
    }
    
    virtual ExprStmt* visitExpressionListExprStmt(ExpressionListExprStmt *node) {
        return new ExpressionListExprStmt(node->sourceSpan(), visitExprStmts(node->expressions()));
    }
    
    virtual ExprStmt* visitNamespaceExprStmt(NamespaceExprStmt *node) {
        return new NamespaceExprStmt(
            node->sourceSpan(),
            visitMultiPartIdentifier(node->qualifiedName()),
            *downcast<ExpressionListExprStmt>(visitExprStmt(&node->body())));
    }
    
    virtual gc_ref_vector<TemplateParameter> visitTemplateParameters(const gc_ref_vector<TemplateParameter> parametersToCopy) {
        gc_ref_vector<TemplateParameter> copiedParameters;
        copiedParameters.reserve(parametersToCopy.size());
        for(TemplateParameter &f : parametersToCopy) {
            copiedParameters.emplace_back(*visitTemplateParameter(&f));
        }
        return copiedParameters;
    }
    
    virtual ExprStmt* visitNamedTemplateExprStmt(NamedTemplateExprStmt *node) {
        return new NamedTemplateExprStmt(
            node->sourceSpan(),
            visitIdentifier(node->name()),
            visitTemplateParameters(node->parameters()),
            *downcast<ExpressionListExprStmt>(visitExprStmt(&node->body())));
    }
    
    virtual ExprStmt* visitAnonymousTemplateExprStmt(AnonymousTemplateExprStmt *node) {
        return new AnonymousTemplateExprStmt(
            node->sourceSpan(),
            visitTemplateParameters(node->parameters()),
            *visitExprStmt(&node->body()));
    }
    
    virtual gc_ref_vector<TypeRef> visitTypeArguments(const gc_ref_vector<TypeRef> &typeArgsToVisit) {
        gc_ref_vector<TypeRef> visited;
        visited.reserve(typeArgsToVisit.size());
        for(auto &arg : typeArgsToVisit) {
            visited.emplace_back(*visitTypeRef(&arg.get()));
        }
        return visited;
    }
    
    virtual gc_ref_vector<ResolutionDeferredTypeRef> visitResolutionDeferredTypeArguments(
        const gc_ref_vector<ResolutionDeferredTypeRef> &typeArgsToVisit) {
        
        //TODO: DRY with regards to above
        gc_ref_vector<ResolutionDeferredTypeRef> visited;
        visited.reserve(typeArgsToVisit.size());
        for(auto &arg : typeArgsToVisit) {
            visited.emplace_back(*downcast<ResolutionDeferredTypeRef>(visitTypeRef(&arg.get())));
        }
        return visited;
    }
    
    virtual ExprStmt* visitTemplateExpansionExprStmt(TemplateExpansionExprStmt *node) {
        return new TemplateExpansionExprStmt(
            node->sourceSpan(),
            visitMultiPartIdentifier(node->templateName()),
            visitTypeArguments(node->typeArguments()));
    
    }
    
    //////////// Misc
    virtual TypeRef* visitResolutionDeferredTypeRef(ResolutionDeferredTypeRef *node) {
        return new ResolutionDeferredTypeRef(
            node->sourceSpan(),
            visitMultiPartIdentifier(node->name()),
            visitResolutionDeferredTypeArguments(node->templateArgTypeRefs()));
    }
    
    virtual TypeRef* visitKnownTypeRef(KnownTypeRef *node) {
        return new KnownTypeRef(
            node->sourceSpan(),
            node->type()); //NOTE: Not visiting this is OK!
    }
    
    virtual TemplateParameter* visitTemplateParameter(TemplateParameter *node) {
        return new TemplateParameter(
            node->sourceSpan(),
            visitIdentifier(node->name()));
    }
    
public:
    //TODO: determine appropriate visibility for this method.
    virtual Module* visitModule(Module *node) {
        return new Module(
            node->name(),
            *downcast<CompoundExprStmt>(visitExprStmt(&node->body())));
    }
};


}}}
