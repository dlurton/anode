#pragma once

namespace anode { namespace front { namespace ast{

enum class AstNodeKind {
    Module, ParameterDef, ExprStmt, TemplateParameter, TypeRef,
};

enum class ExprStmtKind {
    Compound,
    ExpressionList,
    FuncDef,
    FuncCall,
    GenericClassDef,
    CompleteClassDef,
    MethodRef,
    If,
    While,
    LiteralBool,
    LiteralInt32,
    LiteralFloat,
    Unary,
    Binary,
    Dot,
    VariableDecl,
    VariableRef,
    Cast,
    New,
    Known,
    Assert,
    Namespace,
    AnonymousTemplate,
    NamedTemplate,
    TemplateExpansion
};

enum class TypeRefKind {
    Known,
    ResolutionDeferred
};


class AstNode;
class Stmt;
class ExprStmt;
class CompoundExprStmt;
class ExpressionListExprStmt;
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
class VariableDeclExprStmt;
class VariableRefExprStmt;
class CastExprStmt;
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

}}}