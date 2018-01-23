#pragma once

#include "anode.h"

namespace anode { namespace front { namespace ast {

enum class AstNodeKind {
    CompoundExpr,
    ExpressionList,
    ParameterDef,
    FuncDefStmt,
    FuncCallExpr,
    ClassDefinition,
    GenericClassDefinition,
    CompleteClassDefinition,
    MethodRefExpr,
    IfExprStmt,
    WhileExpr,
    LiteralBoolExpr,
    LiteralInt32Expr,
    LiteralFloatExpr,
    UnaryExpr,
    BinaryExpr,
    DotExpr,
    VariableDeclExpr,
    VariableRefExpr,
    CastExpr,
    NewExpr,
    ResolutionDeferredTypeRef,
    KnownTypeRef,
    AssertExprStmt,
    NamespaceExpr,
    TemplateParameter,
    AnonymousTemplateExprStmt,
    NamedTemplateExprStmt,
    TemplateExpansionExprStmt,
    Module,
    AnodeWorld
};

class AstNode;
class Stmt;
class ExprStmt;
class CompoundExpr;
class ExpressionList;
class ParameterDef;
class FuncDefStmt;
class FuncCallExpr;
class ClassDefinition;
class GenericClassDefinition;
class CompleteClassDefinition;
class MethodRefExpr;
class IfExprStmt;
class WhileExpr;
class LiteralBoolExpr;
class LiteralInt32Expr;
class LiteralFloatExpr;
class UnaryExpr;
class BinaryExpr;
class DotExpr;
class VariableDeclExpr;
class VariableRefExpr;
class CastExpr;
class NewExpr;
class ResolutionDeferredTypeRef;
class KnownTypeRef;
class AssertExprStmt;
class NamespaceExpr;
class TemplateParameter;
class AnonymousTemplateExprStmt;
class NamedTemplateExprStmt;
class TemplateExpansionExprStmt;
class Module;
class AnodeWorld;

class RewritingAstVisitor : public gc {
public:
    virtual AstNode *dispatch(AstNode *node) {
        if(auto module = dynamic_cast<AstNode*>(module)) {
            return vissit(module)
        }
    }
    
protected:
    
    
    virtual ParameterDef *visit(ParameterDef *node) {
        return new ParameterDef(node->span(), visit(node.name(), visit(typeRef)))
    }
    
    virtual void visit(FuncDefStmt *node) { }
    
    virtual void visit(FuncCallExpr *node) {  }
    
    virtual void visit(GenericClassDefinition *node) { }
    
    virtual void visit(CompleteClassDefinition *node) { }
    
    virtual void visit(AssertExprStmt *node) { }
    
    virtual void visit(VariableDeclExpr *node) { }
    
    virtual void visit(IfExprStmt *node) { }
    
    virtual void visit(WhileExpr *node) { }
    
    virtual void visit(BinaryExpr *node) { }
    
    virtual void visit(UnaryExpr *node) { }
    
    virtual void visit(LiteralBoolExpr *node) { }
    
    virtual void visit(LiteralInt32Expr *node) { }
    
    virtual void visit(LiteralFloatExpr *node) { }
    
    virtual void visit(VariableRefExpr *node) { }
    
    virtual void visit(MethodRefExpr *node) { }
    
    virtual void visit(CastExpr *node) { }
    
    virtual void visit(NewExpr *node) { }
    
    virtual void visit(DotExpr *node) { }
    
    virtual void visit(CompoundExpr *node) { }
    
    virtual void visit(ExpressionList *node) { }
    
    virtual void visit(NamespaceExpr *node) { }
    
    virtual void visit(NamedTemplateExprStmt *node) { }
    
    virtual void visit(AnonymousTemplateExprStmt *node) { }
    
    virtual void visit(TemplateExpansionExprStmt *node) { }
    
    //////////// Misc
    virtual void visit(ResolutionDeferredTypeRef *node) { }
    
    virtual void visit(KnownTypeRef *node) { }
    
    virtual void visit(TemplateParameter *node) { }
    virtual void visit(Module *node) { }
    
};


}}}