#pragma once

#include "anode.h"

namespace anode { namespace front { namespace ast {

enum class AstNodeKind {
    Module, ParameterDef, ExprStmt, TemplateParameter, TypeRef,
};

enum class ExprStmtKind {
    CompoundExpr,
    ExpressionList,
    FuncDef,
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
    TemplateExpansionExprStmt
};

enum class TypeRefKind {
    Known,
    ResolutionDeferred
};


class AstNode;
class Stmt;
class ExprStmt;
class CompoundExprStmt;
class ExpressionListStmt;
class ParameterDef;
class FuncDefExprStmt;
class FuncCallExprStmt;
class ClassDefinition;
class GenericClassDefExprStmt;
class CompleteClassDefExprStmt;
class MethodRefExprStmt;
class IfExprStmt;
class WhileExprStmt;
class LiteralBoolExprStmt;
class LiteralInt32ExprStmt;
class LiteralFloatExprStmt;
class UnaryExprStmt;
class BinaryExprStmt;
class DotExprStmt;
class VariableDeclExpr;
class VariableRefExprStmt;
class CastExprStmtStmt;
class NewExprStmt;
class ResolutionDeferredTypeRef;
class KnownTypeRef;
class AssertExprStmt;
class NamespaceExprStmt;
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
    
    virtual void visit(FuncDefExprStmt *node) { }
    
    virtual void visit(FuncCallExprStmt *node) {  }
    
    virtual void visit(GenericClassDefExprStmt *node) { }
    
    virtual void visit(CompleteClassDefExprStmt *node) { }
    
    virtual void visit(AssertExprStmt *node) { }
    
    virtual void visit(VariableDeclExpr *node) { }
    
    virtual void visit(IfExprStmt *node) { }
    
    virtual void visit(WhileExprStmt *node) { }
    
    virtual void visit(BinaryExprStmt *node) { }
    
    virtual void visit(UnaryExprStmt *node) { }
    
    virtual void visit(LiteralBoolExprStmt *node) { }
    
    virtual void visit(LiteralInt32ExprStmt *node) { }
    
    virtual void visit(LiteralFloatExprStmt *node) { }
    
    virtual void visit(VariableRefExprStmt *node) { }
    
    virtual void visit(MethodRefExprStmt *node) { }
    
    virtual void visit(CastExprStmtStmt *node) { }
    
    virtual void visit(NewExprStmt *node) { }
    
    virtual void visit(DotExprStmt *node) { }
    
    virtual void visit(CompoundExprStmt *node) { }
    
    virtual void visit(ExpressionListStmt *node) { }
    
    virtual void visit(NamespaceExprStmt *node) { }
    
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