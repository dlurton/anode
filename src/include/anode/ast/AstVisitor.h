#pragma once

namespace anode { namespace front { namespace ast {

class AstNode;
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

class AstVisitor : public gc {
public:
    virtual bool shouldVisitChildren() { return true; }
    
    virtual void beforeAccept(AstNode &) { }
    virtual void afterAccept(AstNode &) { }
    
    //////////// Statements
    virtual void visitingParameterDef(ParameterDef &) { }
    virtual void visitedParameterDef(ParameterDef &) { }
    
    virtual void visitingFuncDefExprStmt(FuncDefExprStmt &) { }
    virtual void visitedFuncDeclStmt(FuncDefExprStmt &) { }
    
    virtual void vistingFuncCallExprStmt(FuncCallExprStmt &) {  }
    virtual void visitedFuncCallExprStmt(FuncCallExprStmt &) { }
    
    virtual void visitingGenericClassDefExprStmt(GenericClassDefExprStmt &) { }
    virtual void visitedGenericClassDefExprStmt(GenericClassDefExprStmt &) { }
    
    virtual void beforeVisit(CompleteClassDefExprStmt &) { }
    virtual void visitedCompleteClassDefExprStmt(CompleteClassDefExprStmt &) { }
    
    //////////// Expressions
    virtual void visitingAssertExprStmt(AssertExprStmt &) { }
    virtual void visitedAssertExprStmt(AssertExprStmt &) { }
    
    virtual void beforeVisit(VariableDeclExpr &) { }
    virtual void visitedVariableDeclExpr(VariableDeclExpr &) { }
    
    virtual void beforeVisit(IfExprStmt &) { }
    virtual void visitedIfExpr(IfExprStmt &) { }
    
    virtual void beforeVisit(WhileExprStmt &) { }
    virtual void visitedWhileExpr(WhileExprStmt &) { }
    
    virtual void beforeVisit(BinaryExprStmt &) { }
    virtual void visitedBinaryExpr(BinaryExprStmt &) { }
    
    virtual void visitingUnaryExpr(UnaryExprStmt &) { }
    virtual void visitedUnaryExpr(UnaryExprStmt &) { }
    
    virtual void visitLiteralBoolExpr(LiteralBoolExprStmt &) { }
    
    virtual void visitLiteralInt32Expr(LiteralInt32ExprStmt &) { }
    
    virtual void visitLiteralFloatExpr(LiteralFloatExprStmt &) { }
    
    virtual void visitVariableRefExpr(VariableRefExprStmt &) { }
    
    virtual void visitMethodRefExpr(MethodRefExprStmt &) { }
    
    virtual void visitingCastExprStmt(CastExprStmtStmt &) { }
    virtual void visitedCastExprStmt(CastExprStmtStmt &) { }
    
    virtual void visitingNewExpr(NewExprStmt &) { }
    virtual void visitedNewExpr(NewExprStmt &) { }
    
    virtual void visitingDotExpr(DotExprStmt &) { }
    virtual void visitedDotExpr(DotExprStmt &) { }
    
    virtual void visitingCompoundExpr(CompoundExprStmt &) { }
    virtual void visitedCompoundExpr(CompoundExprStmt &) { }
    
    virtual void visitingExpressionList(ExpressionListStmt &) { }
    virtual void visitedExpressionList(ExpressionListStmt &) { }
    
    virtual void visitingNamespaceExpr(NamespaceExprStmt &) { }
    virtual void visitedNamespaceExpr(NamespaceExprStmt &) { }
    
    virtual void visitingNamedTemplateExprStmt(NamedTemplateExprStmt &) { }
    virtual void visitedNamedTemplateExprStmt(NamedTemplateExprStmt &) { }
    
    virtual void visitingAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &) { }
    virtual void visitedAnonymousTemplateExprStmt(AnonymousTemplateExprStmt &) { }
    
    virtual void visitingTemplateExpansionExprStmt(TemplateExpansionExprStmt &) { }
    virtual void visitedTemplateExpansionExprStmt(TemplateExpansionExprStmt &) { }
    
    //////////// Misc
    virtual void visitedResolutionDeferredTypeRef(ResolutionDeferredTypeRef &) { }
    
    virtual void visitKnownTypeRef(KnownTypeRef &) { }
    
    virtual void visitTemplateParameter(TemplateParameter &) { }
    virtual void visitingModule(Module &) { }
    
    virtual void visitedModule(Module &) { }
};


}}}