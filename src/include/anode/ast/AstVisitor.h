#pragma once

namespace anode { namespace front { namespace ast {

class AstNode;
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

class AstVisitor : public gc {
public:
    virtual bool shouldVisitChildren() { return true; }
    
    virtual void beforeAccept(AstNode &) { }
    virtual void afterAccept(AstNode &) { }
    
    //////////// Statements
    virtual void visitingParameterDef(ParameterDef &) { }
    virtual void visitedParameterDef(ParameterDef &) { }
    
    virtual void visitingFuncDefStmt(FuncDefStmt &) { }
    virtual void visitedFuncDeclStmt(FuncDefStmt &) { }
    
    virtual void visitingFuncCallExpr(FuncCallExpr &) {  }
    virtual void visitedFuncCallExpr(FuncCallExpr &) { }
    
    //    virtual void visitingClassDefinition(ClassDefinition &) { }
    //    virtual void visitedClassDefinition(ClassDefinition &) { }
    
    virtual void visitingGenericClassDefinition(GenericClassDefinition &) { }
    virtual void visitedGenericClassDefinition(GenericClassDefinition &) { }
    
    virtual void visitingCompleteClassDefinition(CompleteClassDefinition &) { }
    virtual void visitedCompleteClassDefinition(CompleteClassDefinition &) { }
    
    //    virtual void visitingReturnStmt(ReturnStmt &) { }
    //    virtual void visitedReturnStmt(ReturnStmt &) { }
    
    //////////// Expressions
    virtual void visitingAssertExprStmt(AssertExprStmt &) { }
    virtual void visitedAssertExprStmt(AssertExprStmt &) { }
    
    virtual void visitingVariableDeclExpr(VariableDeclExpr &) { }
    virtual void visitedVariableDeclExpr(VariableDeclExpr &) { }
    
    virtual void visitingIfExpr(IfExprStmt &) { }
    virtual void visitedIfExpr(IfExprStmt &) { }
    
    virtual void visitingWhileExpr(WhileExpr &) { }
    virtual void visitedWhileExpr(WhileExpr &) { }
    
    virtual void visitingBinaryExpr(BinaryExpr &) { }
    virtual void visitedBinaryExpr(BinaryExpr &) { }
    
    virtual void visitingUnaryExpr(UnaryExpr &) { }
    virtual void visitedUnaryExpr(UnaryExpr &) { }
    
    virtual void visitLiteralBoolExpr(LiteralBoolExpr &) { }
    
    virtual void visitLiteralInt32Expr(LiteralInt32Expr &) { }
    
    virtual void visitLiteralFloatExpr(LiteralFloatExpr &) { }
    
    virtual void visitVariableRefExpr(VariableRefExpr &) { }
    
    virtual void visitMethodRefExpr(MethodRefExpr &) { }
    
    virtual void visitingCastExpr(CastExpr &) { }
    virtual void visitedCastExpr(CastExpr &) { }
    
    virtual void visitingNewExpr(NewExpr &) { }
    virtual void visitedNewExpr(NewExpr &) { }
    
    virtual void visitingDotExpr(DotExpr &) { }
    virtual void visitedDotExpr(DotExpr &) { }
    
    virtual void visitingCompoundExpr(CompoundExpr &) { }
    virtual void visitedCompoundExpr(CompoundExpr &) { }
    
    virtual void visitingExpressionList(ExpressionList &) { }
    virtual void visitedExpressionList(ExpressionList &) { }
    
    virtual void visitingNamespaceExpr(NamespaceExpr &) { }
    virtual void visitedNamespaceExpr(NamespaceExpr &) { }
    
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