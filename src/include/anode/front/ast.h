#pragma once
#pragma once

#include "anode.h"
#include "front/unique_id.h"
#include "type.h"
#include "scope.h"
#include "source.h"

#include <string>
#include <functional>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>



namespace anode { namespace front { namespace ast {

enum class BinaryOperationKind : unsigned char {
    Assign,
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    NotEq,
    LogicalAnd,
    LogicalOr,
    GreaterThan,
    LessThan,
    GreaterThanOrEqual,
    LessThanOrEqual
};
std::string to_string(BinaryOperationKind kind);

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
//class ReturnStmt;
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
class TemplateParameter;
class TemplateExprStmt;
class TemplateExpansionExprStmt;
class Module;
class AnodeWorld;

class AstVisitor : public gc {
public:
    virtual bool shouldVisitChildren() { return true; }

    //////////// Statements
    virtual void visitingParameterDef(ParameterDef *) { }
    virtual void visitedParameterDef(ParameterDef *) { }

    virtual void visitingFuncDefStmt(FuncDefStmt *) { }
    virtual void visitedFuncDeclStmt(FuncDefStmt *) { }

    virtual void visitingFuncCallExpr(FuncCallExpr *) {  }
    virtual void visitedFuncCallExpr(FuncCallExpr *) { }

//    virtual void visitingClassDefinition(ClassDefinition *) { }
//    virtual void visitedClassDefinition(ClassDefinition *) { }

    virtual void visitingGenericClassDefinition(GenericClassDefinition *) { }
    virtual void visitedGenericClassDefinition(GenericClassDefinition *) { }

    virtual void visitingCompleteClassDefinition(CompleteClassDefinition *) { }
    virtual void visitedCompleteClassDefinition(CompleteClassDefinition *) { }

//    virtual void visitingReturnStmt(ReturnStmt *) { }
//    virtual void visitedReturnStmt(ReturnStmt *) { }

    //////////// Expressions
    virtual void visitingAssertExprStmt(AssertExprStmt *) { }
    virtual void visitedAssertExprStmt(AssertExprStmt *) { }

    virtual void visitingVariableDeclExpr(VariableDeclExpr *) { }
    virtual void visitedVariableDeclExpr(VariableDeclExpr *) { }

    virtual void visitingIfExpr(IfExprStmt *) { }
    virtual void visitedIfExpr(IfExprStmt *) { }

    virtual void visitingWhileExpr(WhileExpr *) { }
    virtual void visitedWhileExpr(WhileExpr *) { }

    virtual void visitingBinaryExpr(BinaryExpr *) { }
    virtual void visitedBinaryExpr(BinaryExpr *) { }

    virtual void visitingUnaryExpr(UnaryExpr *) { }
    virtual void visitedUnaryExpr(UnaryExpr *) { }

    virtual void visitLiteralBoolExpr(LiteralBoolExpr *) { }

    virtual void visitLiteralInt32Expr(LiteralInt32Expr *) { }

    virtual void visitLiteralFloatExpr(LiteralFloatExpr *) { }

    virtual void visitVariableRefExpr(VariableRefExpr *) { }

    virtual void visitMethodRefExpr(MethodRefExpr *) { }

    virtual void visitingCastExpr(CastExpr *) { }
    virtual void visitedCastExpr(CastExpr *) { }

    virtual void visitingNewExpr(NewExpr *) { }
    virtual void visitedNewExpr(NewExpr *) { }

    virtual void visitingDotExpr(DotExpr *) { }
    virtual void visitedDotExpr(DotExpr *) { }

    virtual void visitingCompoundExpr(CompoundExpr *) { }
    virtual void visitedCompoundExpr(CompoundExpr *) { }

    virtual void visitingExpressionList(ExpressionList *) { }
    virtual void visitedExpressionList(ExpressionList *) { }

    virtual void visitingTemplateExprStmt(TemplateExprStmt *) { }
    virtual void visitedTemplateExprStmt(TemplateExprStmt *) { }

    virtual void visitingTemplateExpansionExprStmt(TemplateExpansionExprStmt *) { }
    virtual void visitedTemplateExpansionExprStmt(TemplateExpansionExprStmt *) { }

    //////////// Misc
    virtual void visitedResolutionDeferredTypeRef(ResolutionDeferredTypeRef *) { }

    virtual void visitKnownTypeRef(KnownTypeRef *) { }

    virtual void visitTemplateParameter(TemplateParameter *) { }
    virtual void visitingModule(Module *) { }

    virtual void visitedModule(Module *) { }
};


extern unsigned long astNodesDestroyedCount;

class Identifier {
    source::SourceSpan span_;
    std::string text_;

public:
    Identifier(source::SourceSpan span, std::string text)
        : span_(span), text_(text) { }

    const std::string &text() const { return text_; }
    const source::SourceSpan &span() const { return span_; }
};

/** Base class for all nodes */
class AstNode : public Object, no_copy, no_assign {
    UniqueId nodeId_;
public:
    AstNode();

    virtual ~AstNode() {
        astNodesDestroyedCount++;
    }

    UniqueId nodeId() { return nodeId_; }

    virtual void accept(AstVisitor *visitor) = 0;
};

/** A statement any kind of non-terminal that may appear within a CompoundStatement (i.e. between { and }), or
 * at the global scope, i.e. global variables, assignments, class definitions,
 */
class Stmt : public AstNode {
protected:
    source::SourceSpan sourceSpan_;
    Stmt(const source::SourceSpan &sourceSpan) : sourceSpan_(sourceSpan) { }
public:
    //virtual StmtKind stmtKind() const = 0;

    source::SourceSpan sourceSpan() const {
        return sourceSpan_;
    }
};

/** AST node representing a reference to a data type. */
class TypeRef : public AstNode {
    source::SourceSpan sourceSpan_;
protected:
    TypeRef(const source::SourceSpan &sourceSpan) : sourceSpan_{sourceSpan} { }
public:
    source::SourceSpan sourceSpan() const {
        return sourceSpan_;
    }

    virtual type::Type *type() const = 0;
    virtual std::string name() const = 0;
    virtual TypeRef *deepCopyForTemplate() const = 0;
};


/** A reference to a data type that is known at parse time (used for primitive data types). */
class KnownTypeRef : public TypeRef {
    type::Type *referencedType_;
public:
    KnownTypeRef(source::SourceSpan sourceSpan, type::Type *dataType)
        : TypeRef(sourceSpan), referencedType_{dataType}
    { }

    type::Type *type() const override { return referencedType_; };
    std::string name() const override { return referencedType_->name(); };

    TypeRef *deepCopyForTemplate() const override {
        return new KnownTypeRef(sourceSpan(), referencedType_);
    }

    void accept(AstVisitor *visitor) override {
        visitor->visitKnownTypeRef(this);
    }
};

template<typename TIterator, typename TItem>
inline std::string join(TIterator begin, TIterator end, std::string delimiter, std::function<std::string(TItem)> toString) {
    if(begin == end) return "";
    std::string output = toString(*(begin++));

    while(begin != end) {
        output += delimiter;
        output += toString(*(begin++));
    }

    return output;
}

/** A reference to a data type that doesn't exist at parse time. i.e.: classes.  Resolved during an AST pass. */
class ResolutionDeferredTypeRef : public TypeRef {
    const std::string name_;
    gc_vector<ResolutionDeferredTypeRef*> templateArgs_;
    type::ResolutionDeferredType *referencedType_;
public:
    ResolutionDeferredTypeRef(const source::SourceSpan &sourceSpan, std::string name, const gc_vector<ResolutionDeferredTypeRef*> &args)
        : TypeRef(sourceSpan), name_{name}, templateArgs_{args}, referencedType_(new type::ResolutionDeferredType()) { }

    std::string name() const override { return name_; }

    type::Type *type() const override {
        ASSERT(referencedType_ && "Data type hasn't been resolved yet");
        return referencedType_;
    }

    bool isResolved() { return referencedType_->isResolved(); }

    bool hasTemplateArguments() { return !templateArgs_.empty(); }
    gc_vector<ResolutionDeferredTypeRef*> templateArgs() { return templateArgs_; }

    void setType(type::Type *referencedType) {
        referencedType_->resolve(referencedType);
    }

    void accept(AstVisitor *visitor) override {
        for(auto ta : templateArgs_) {
            ta->accept(visitor);
        }
        visitor->visitedResolutionDeferredTypeRef(this);
    }

    TypeRef* deepCopyForTemplate() const override {
        gc_vector<ResolutionDeferredTypeRef*> templateArgs;
        for(auto ta : templateArgs_) {
            templateArgs.push_back(upcast<ResolutionDeferredTypeRef>(ta->deepCopyForTemplate()));
        }

        return new ResolutionDeferredTypeRef(sourceSpan(), name_, templateArgs);
    }
};


template<typename TItem>
gc_vector<TItem> deepCopyVector(const gc_vector<TItem> copyFrom) {
    gc_vector<TItem> copy;
    for(auto item : copyFrom) {
        copy.push_back(item->deepCopyForTemplate());
    }
    return copy;
}

/** Represents an instance of a template argument. */
class TemplateArgument {
    std::string parameterName_;
    TypeRef *typeRef_;
public:
    TemplateArgument(const std::string &parameterName, TypeRef* typeRef) : parameterName_{parameterName}, typeRef_{typeRef} { }
    std::string parameterName() { return parameterName_; }
    TypeRef* typeRef() const { return typeRef_; }
};

typedef gc_vector<TemplateArgument*> TemplateArgVector;

/** Base class for all expressions. */
class ExprStmt : public Stmt {
protected:
    explicit ExprStmt(const source::SourceSpan &sourceSpan) : Stmt(sourceSpan) { }
public:
    ~ExprStmt() override = default;

    virtual type::Type *type() const  = 0;
    virtual bool canWrite() const = 0;
    virtual ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const = 0;
};



/** Like CompoundExpr but does not create a lexical scope. */
class ExpressionList : public ExprStmt {
    gc_vector<ExprStmt*> expressions_;
public:
    ExpressionList(source::SourceSpan sourceSpan, const gc_vector<ExprStmt*> &expressions)
        : ExprStmt(sourceSpan),
          expressions_{expressions}
    {
#ifdef ANODE_DEBUG
        for(auto e : expressions_) {
            ASSERT(e && "At least one expression was nullptr");
        }
#endif
    }

    virtual type::Type *type() const override {
        ASSERT(expressions_.size() > 0);
        return expressions_.back()->type();
    };

    bool canWrite() const override { return false; };

    gc_vector<ExprStmt*> expressions() const {
        return expressions_;
    }

    void append(ExprStmt *exprStmt) {
        expressions_.push_back(exprStmt);
    }

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExpressionList(this);

        if(visitor->shouldVisitChildren()) {
            //Copy expressions_ because expressions can be added while visiting children
            //(adding items to vector invalidates iterator)
            gc_vector<ExprStmt*> exprs = expressions_;
            for (auto stmt : exprs) {
                stmt->accept(visitor);
            }
        }
        visitor->visitedExpressionList(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        gc_vector<ExprStmt*> clonedExprs;
        clonedExprs.reserve(expressions_.size());
        for(auto exprStmt : expressions_) {
            clonedExprs.push_back(exprStmt->deepCopyExpandTemplate(templateArgs));
        }
        return new ExpressionList(sourceSpan_, clonedExprs);
    }

};

/** Contains a series of expressions within a lexical scope, i.e. those contained within { and }.
 * TODO: inherit from ExpressionList? */
class CompoundExpr : public ExprStmt {
    scope::SymbolTable scope_;
    gc_vector<ExprStmt*> expressions_;
public:
    CompoundExpr(
        source::SourceSpan sourceSpan,
        scope::StorageKind storageKind,
        const gc_vector<ExprStmt*> &expressions)
        : ExprStmt(sourceSpan),
          scope_{storageKind},
          expressions_{expressions}
    {
#ifdef ANODE_DEBUG
        for(auto e : expressions_) {
            ASSERT(e && "At least one expression was nullptr");
        }
#endif
    }

    scope::SymbolTable *scope() { return &scope_; }

    virtual type::Type *type() const override {
        ASSERT(expressions_.size() > 0);
        return expressions_.back()->type();
    };

    bool canWrite() const override { return false; };

    gc_vector<ExprStmt*> expressions() const {
        return expressions_;
    }

    void append(ExprStmt *exprStmt) {
        expressions_.push_back(exprStmt);
    }

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingCompoundExpr(this);

        if(visitor->shouldVisitChildren()) {
            //Copy expressions_ because expressions can be added while visiting children
            //(adding items to vector invalidates iterator)
            gc_vector<ExprStmt*> exprs = expressions_;
            for (auto stmt : exprs) {
                stmt->accept(visitor);
            }
        }
        visitor->visitedCompoundExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        gc_vector<ExprStmt*> clonedExprs;
        clonedExprs.reserve(expressions_.size());
        for(auto exprStmt : expressions_) {
            clonedExprs.push_back(exprStmt->deepCopyExpandTemplate(templateArgs));
        }
        return new CompoundExpr(sourceSpan_, scope_.storageKind(), clonedExprs);
    }

};

class TemplateParameter : public Stmt {
    const std::string name_;
public:
    TemplateParameter(const source::SourceSpan &sourceSpan, const std::string &name) : Stmt(sourceSpan), name_{name} { }

    std::string name() const { return name_; }

    void accept(AstVisitor *visitor) override {
        visitor->visitTemplateParameter(this);
    }

    TemplateParameter *deepCopyForTemplate() const {
        return new TemplateParameter(sourceSpan_, name());
    }
};

class TemplateExprStmt : public ExprStmt {
    std::string name_;
    gc_vector<ast::TemplateParameter*> parameters_;
    ast::ExpressionList *body_;
public:
    TemplateExprStmt(
        const source::SourceSpan &sourceSpan,
        std::string name,
        const gc_vector<ast::TemplateParameter*> &parameters,
        ast::ExpressionList *body)
        : ExprStmt(sourceSpan),
          name_{name},
          parameters_{parameters},
          body_{body} {}

    gc_vector<ast::TemplateParameter*> &parameters() { return parameters_; }

    type::Type *type() const override { return &type::Primitives::Void; }
    bool canWrite() const override { return false; };

    std::string name() { return name_; }
    ExpressionList *body() { return body_; }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        gc_vector<ast::TemplateParameter*> copiedParameters;
        copiedParameters.reserve(parameters_.size());
        for(auto f : parameters_) {
            copiedParameters.push_back(f->deepCopyForTemplate());
        }

        return new TemplateExprStmt(sourceSpan_, name_, copiedParameters, upcast<ExpressionList>(body_->deepCopyExpandTemplate(templateArgs)));
    }

    void accept(AstVisitor *visitor) override {
        visitor->visitingTemplateExprStmt(this);

        for(auto p : parameters_) {
            p->accept(visitor);
        }

        //Note that we do *not* visit body_ here.
        visitor->visitedTemplateExprStmt(this);
    }
};

class TemplateExpansionExprStmt : public ExprStmt {
    Identifier templateName_;
    //TODO:  convert below to vector of TemplateArgument*
    gc_vector<ast::TypeRef*> typeArguments_;
    ast::ExprStmt *expandedTemplate_ = nullptr;
    scope::SymbolTable templateParameterScope_;
public:
    TemplateExpansionExprStmt(
        source::SourceSpan sourceSpan,
        const Identifier &templateName,
        gc_vector<ast::TypeRef*> typeArguments)
        : ExprStmt(sourceSpan),
          templateName_{templateName},
          typeArguments_{typeArguments},
          templateParameterScope_{scope::StorageKind::TemplateParameter}
    { }

    type::Type *type() const override { return &type::Primitives::Void; }
    virtual bool canWrite() const override { return false; };

    const Identifier &templatedId() { return templateName_; }
    gc_vector<ast::TypeRef*> typeArguments() { return typeArguments_; }

    scope::SymbolTable *templateParameterScope() { return &templateParameterScope_; }

    void setExpandedTemplate(ExprStmt *expandedTemplate) { expandedTemplate_ = expandedTemplate; }
    ast::ExprStmt *expandedTemplate() { return expandedTemplate_; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingTemplateExpansionExprStmt(this);
        if(visitor->shouldVisitChildren()) {
            for(auto arg : typeArguments_) {
                arg->accept(visitor);
            }
            if(expandedTemplate_) {
                expandedTemplate_->accept(visitor);
            }
        }
        visitor->visitedTemplateExpansionExprStmt(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new TemplateExpansionExprStmt(sourceSpan_, templateName_, deepCopyVector(typeArguments_));
    }
};


/** Represents a literal boolean */
class LiteralBoolExpr : public ExprStmt {
    bool const value_;
public:
    LiteralBoolExpr(source::SourceSpan sourceSpan, const bool value) : ExprStmt(sourceSpan), value_(value) {}
    type::Type *type() const override { return &type::Primitives::Bool; }
    bool value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitLiteralBoolExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new LiteralBoolExpr(sourceSpan_, value_);
    }
};

/** Represents an expression that is a literal 32 bit integer. */
class LiteralInt32Expr : public ExprStmt {
    int const value_;
public:
    LiteralInt32Expr(source::SourceSpan sourceSpan, const int value) : ExprStmt(sourceSpan), value_(value) {}
    type::Type *type() const override { return &type::Primitives::Int32; }
    int value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitLiteralInt32Expr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new LiteralInt32Expr(sourceSpan_, value_);
    }
};

/** Represents an expression that is a literal float. */
class LiteralFloatExpr : public ExprStmt {
    float const value_;
public:
    LiteralFloatExpr(source::SourceSpan sourceSpan, const float value) : ExprStmt(sourceSpan), value_(value) {}

    type::Type *type() const override {  return &type::Primitives::Float; }
    float value() const { return value_; }

    bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitLiteralFloatExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new LiteralFloatExpr(sourceSpan_, value_);
    }

};

enum class UnaryOperationKind : unsigned char {
    Not,
    PreIncrement,
    PreDecrement
};
std::string to_string(UnaryOperationKind type);

class UnaryExpr : public ExprStmt {
    const source::SourceSpan operatorSpan_;
    ExprStmt *valueExpr_;
    const UnaryOperationKind operation_;

public:
    /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
    UnaryExpr(source::SourceSpan sourceSpan, ExprStmt *valueExpr, UnaryOperationKind operation, source::SourceSpan operatorSpan)
        : ExprStmt{sourceSpan}, operatorSpan_{operatorSpan}, valueExpr_{valueExpr}, operation_{operation} {

        ASSERT(valueExpr_);
    }

    source::SourceSpan operatorSpan() { return operatorSpan_; }

    type::Type *type() const override { return &type::Primitives::Bool; }

    ExprStmt *valueExpr() const { return valueExpr_; }
    void setLValue(ExprStmt *newLValue) { valueExpr_ = newLValue; }

    virtual bool canWrite() const override { return false; };

    UnaryOperationKind operation() const { return operation_; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingUnaryExpr(this);

        if(visitor->shouldVisitChildren()) {
            valueExpr_->accept(visitor);
        }
        visitor->visitedUnaryExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new UnaryExpr(sourceSpan_, valueExpr_->deepCopyExpandTemplate(templateArgs), operation_, operatorSpan_);
    }
};

/** Represents the type of binary operation.
 * Distinction between these binary operation types are important because although they
 * use the same data structure to represent them, how their code is emitted is very
 * different.
 */
enum class BinaryExprKind : unsigned char {
    /** Both operands of an arithmetic binary expression are always evaluated. */
        Arithmetic,
    /** At least one operand of a logical operation is always evaluated and one
     * may be skipped due to short-circuiting. */
        Logical
};

/** Represents a binary expression, i.e. 1 + 2 or foo + bar */
class BinaryExpr : public ExprStmt {
    ExprStmt *lValue_;
    const BinaryOperationKind operation_;
    const source::SourceSpan operatorSpan_;
    ExprStmt *rValue_;

public:
    /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
    BinaryExpr(source::SourceSpan sourceSpan,
               ExprStmt *lValue,
               BinaryOperationKind operation,
               source::SourceSpan operatorSpan,
               ExprStmt* rValue)
        : ExprStmt{sourceSpan},
          lValue_{lValue},
          operation_{operation},
          operatorSpan_{operatorSpan},
          rValue_{rValue} {

        ASSERT(lValue_);
        ASSERT(rValue_);
    }

    source::SourceSpan operatorSpan() { return operatorSpan_; }

    /** This is the type of the result, which may be different than the type of the operands depending on the operation type,
     * because some operation types (e.g. equality, logical and, or, etc) always yield boolean values.  */
    type::Type *type() const override {
        if(isComparison()) {
            return &type::Primitives::Bool;
        }
        return operandsType();
    }

    bool isComparison() const {
        switch(operation_) {
            case BinaryOperationKind::Eq:
            case BinaryOperationKind::NotEq:
            case BinaryOperationKind::GreaterThan:
            case BinaryOperationKind::GreaterThanOrEqual:
            case BinaryOperationKind::LessThan:
            case BinaryOperationKind::LessThanOrEqual:
                return true;
            default:
                return false;
        }
    }

    /** This is the type of the operands. */
    type::Type *operandsType() const {
        ASSERT(rValue_->type()->isSameType(lValue_->type()->actualType()));
        return rValue_->type();
    }

    ExprStmt *lValue() const { return lValue_; }
    void setLValue(ExprStmt *newLValue) { lValue_ = newLValue; }

    ExprStmt *rValue() const { return rValue_; }
    void setRValue(ExprStmt *newRValue) { rValue_ = newRValue; }

    virtual bool canWrite() const override { return false; };

    BinaryOperationKind operation() const { return operation_; }

    BinaryExprKind binaryExprKind() {
        switch(operation_) {
            case BinaryOperationKind::Assign:
            case BinaryOperationKind::Eq:
            case BinaryOperationKind::NotEq:
            case BinaryOperationKind::Mul:
            case BinaryOperationKind::Add:
            case BinaryOperationKind::Sub:
            case BinaryOperationKind::Div:
            case BinaryOperationKind::LessThan:
            case BinaryOperationKind::LessThanOrEqual:
            case BinaryOperationKind::GreaterThan:
            case BinaryOperationKind::GreaterThanOrEqual:
                return BinaryExprKind::Arithmetic;
            case BinaryOperationKind::LogicalOr:
            case BinaryOperationKind::LogicalAnd:
                return BinaryExprKind::Logical;
            default:
                ASSERT_FAIL("Unknown binary operation kind");
        }
    }

    void accept(AstVisitor *visitor) override {
        visitor->visitingBinaryExpr(this);

        if(visitor->shouldVisitChildren()) {
            lValue_->accept(visitor);
            rValue_->accept(visitor);
        }
        visitor->visitedBinaryExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new BinaryExpr(sourceSpan_,
                              lValue_->deepCopyExpandTemplate(templateArgs),
                              operation_,
                              operatorSpan_,
                              rValue_->deepCopyExpandTemplate(templateArgs));
    }
};

enum class VariableAccess : unsigned char {
    Read,
    Write
};

/** Represents a reference to a previously declared variable. */
class VariableRefExpr : public ExprStmt {
    scope::Symbol *symbol_ = nullptr;
    VariableAccess access_ = VariableAccess::Read;
protected:
    std::string name_;

public:
    VariableRefExpr(source::SourceSpan sourceSpan, const std::string &name, VariableAccess access = VariableAccess::Read)
        : ExprStmt(sourceSpan), access_{access}, name_{ name } {
        ASSERT(name.size() > 0);
    }

    virtual type::Type *type() const override {
        ASSERT(symbol_);
        return symbol_->type();
    }

    std::string name() const { return name_; }
    std::string toString() const { return name_ + ":" + this->type()->nameForDisplay(); }

    scope::Symbol *symbol() {
        return symbol_;
    }
    void setSymbol(scope::Symbol *symbol) {
        symbol_ = symbol;
    }

    bool canWrite() const override { return true; };

    VariableAccess variableAccess() const { return access_; }
    void setVariableAccess(VariableAccess access) { access_ = access; }

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitVariableRefExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        VariableRefExpr *varRef = new VariableRefExpr(sourceSpan_, name_, access_);
        varRef->access_ = access_;
        return varRef;
    }

};


/** Defines a variable and references it. */
class VariableDeclExpr : public VariableRefExpr {
    TypeRef* typeRef_;
public:
    VariableDeclExpr(source::SourceSpan sourceSpan, const std::string &name, TypeRef* typeRef, VariableAccess access = VariableAccess::Read)
        : VariableRefExpr(sourceSpan, name, access),
          typeRef_(typeRef)
    {
    }

    TypeRef *typeRef() { return typeRef_; }
    virtual type::Type *type() const override { return typeRef_->type(); }

    virtual bool canWrite() const override { return true; };

    void accept(AstVisitor *visitor) override {
        visitor->visitingVariableDeclExpr(this);

        if(visitor->shouldVisitChildren()) {
            typeRef_->accept(visitor);
        }

        visitor->visitedVariableDeclExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new VariableDeclExpr(sourceSpan_, name_, typeRef_->deepCopyForTemplate(), variableAccess());
    }
};

enum class CastKind : unsigned char {
    Explicit,
    Implicit
};

/** Represents a cast expression... i.e. foo:int= cast<int>(someDouble); */
class CastExpr : public ExprStmt {
    TypeRef  *toType_;
    ExprStmt *valueExpr_;
    const CastKind castKind_;

public:
    /** Use this constructor when the type::Type of the cast *is* known in advance. */
    CastExpr(source::SourceSpan sourceSpan, type::Type *toType, ExprStmt* valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toType_(new KnownTypeRef(sourceSpan, toType)), valueExpr_(valueExpr), castKind_(castKind)
    {
        ASSERT(toType);
        ASSERT(valueExpr);
    }

    /** Use this constructor when the type::Type of the cast is *not* known in advance. */
    CastExpr(source::SourceSpan sourceSpan, TypeRef *toType, ExprStmt *valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toType_(toType), valueExpr_(valueExpr), castKind_(castKind) { }

    static CastExpr *createImplicit(ExprStmt *valueExpr, type::Type *toType);

    type::Type *type() const  override{ return toType_->type(); }
    CastKind castKind() const { return castKind_; }

    virtual bool canWrite() const override { return false; };

    ExprStmt *valueExpr() const { return valueExpr_; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingCastExpr(this);

        if(visitor->shouldVisitChildren()) {
            toType_->accept(visitor);
            valueExpr_->accept(visitor);
        }

        visitor->visitedCastExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new CastExpr(sourceSpan_, toType_->deepCopyForTemplate(), valueExpr_->deepCopyExpandTemplate(templateArgs), castKind_);
    }
};

/** Represents a new expression... i.e. foo:int= new<int>(someDouble); */
class NewExpr : public ExprStmt {
    TypeRef  *typeRef_;
public:

    NewExpr(source::SourceSpan sourceSpan, TypeRef *typeRef)
        : ExprStmt(sourceSpan), typeRef_(typeRef)
    {
        ASSERT(typeRef);
    }

    type::Type *type() const  override { return typeRef_->type(); }
    TypeRef *typeRef() const { return typeRef_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitingNewExpr(this);

        if(visitor->shouldVisitChildren()) {
            typeRef_->accept(visitor);
        }

        visitor->visitedNewExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new NewExpr(sourceSpan_, typeRef_->deepCopyForTemplate());
    }
};

/** Can be the basis of an if-then-else or ternary operator. */
class IfExprStmt : public ExprStmt {
    ExprStmt* condition_;
    ExprStmt* thenExpr_;
    ExprStmt* elseExpr_;
public:

    /** Note:  assumes ownership of condition, truePart and falsePart.  */
    IfExprStmt(source::SourceSpan sourceSpan,
               ExprStmt* condition,
               ExprStmt* trueExpr,
               ExprStmt* elseExpr)
        : ExprStmt(sourceSpan),
          condition_{ condition },
          thenExpr_{ trueExpr },
          elseExpr_{ elseExpr } {
        ASSERT(condition_);
        ASSERT(thenExpr_);
    }

    type::Type *type() const override {
        if(elseExpr_ == nullptr || !thenExpr_->type()->isSameType(elseExpr_->type())) {
            return &type::Primitives::Void;
        }

        return thenExpr_->type();
    }

    ExprStmt *condition() const { return condition_; }
    void setCondition(ExprStmt *newCondition){ condition_ = newCondition; }

    ExprStmt *thenExpr() const { return thenExpr_; }
    void setThenExpr(ExprStmt * newThenExpr) { thenExpr_ = newThenExpr; }

    ExprStmt *elseExpr() const { return elseExpr_; }
    void setElseExpr(ExprStmt *newElseExpr) { elseExpr_ = newElseExpr; }

    bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitingIfExpr(this);

        if(visitor->shouldVisitChildren()) {
            condition_->accept(visitor);
            thenExpr_->accept(visitor);
            if(elseExpr_)
                elseExpr_->accept(visitor);
        }

        visitor->visitedIfExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new IfExprStmt(
            sourceSpan_,
            condition_->deepCopyExpandTemplate(templateArgs),
            thenExpr_->deepCopyExpandTemplate(templateArgs),
            elseExpr_->deepCopyExpandTemplate(templateArgs));
    }
};

class WhileExpr : public ExprStmt {
    ExprStmt* condition_;
    ExprStmt* body_;
public:

    /** Note:  assumes ownership of condition, truePart and falsePart.  */
    WhileExpr(source::SourceSpan sourceSpan,
              ExprStmt* condition,
              ExprStmt* body)
        : ExprStmt(sourceSpan),
          condition_{ condition },
          body_{ body } {
        ASSERT(condition_);
        ASSERT(body_)
    }

    type::Type *type() const override {
        //For now, while expressions will not return a value.
        return &type::Primitives::Void;
    }

    ExprStmt *condition() const { return condition_; }

    void setCondition(ExprStmt *newCondition) { condition_ = newCondition; }

    ExprStmt *body() const { return body_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor *visitor) override {
        visitor->visitingWhileExpr(this);

        if(visitor->shouldVisitChildren()) {
            condition_->accept(visitor);
            body_->accept(visitor);
        }

        visitor->visitedWhileExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new WhileExpr(sourceSpan_, condition_->deepCopyExpandTemplate(templateArgs), body_->deepCopyExpandTemplate(templateArgs));
    }
};

class ParameterDef : public AstNode {
    source::SourceSpan span_;
    const std::string name_;
    TypeRef* typeRef_;
    scope::VariableSymbol *symbol_ = nullptr;
public:
    ParameterDef(source::SourceSpan span, const std::string &name, TypeRef *typeRef)
        : span_{span}, name_{name}, typeRef_{typeRef} { }

    source::SourceSpan span() { return span_; }
    std::string name() { return name_; }
    type::Type *type() { return typeRef_->type(); }
    TypeRef *typeRef() { return typeRef_; }

    scope::VariableSymbol *symbol() {
        ASSERT(symbol_);
        return symbol_;
    }
    void setSymbol(scope::VariableSymbol *symbol) { symbol_ = symbol; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingParameterDef(this);
        if(visitor->shouldVisitChildren()) {
            typeRef_->accept(visitor);
        }
        visitor->visitedParameterDef(this);
    }

    ParameterDef *deepCopy() {
        return new ParameterDef(span_, name_, typeRef_->deepCopyForTemplate());
    }
};
type::FunctionType *createFunctionType(type::Type *returnType, const gc_vector<ParameterDef*> &parameters);

class FuncDefStmt : public ExprStmt {
    const std::string name_;
    scope::FunctionSymbol *symbol_= nullptr;
    scope::SymbolTable parameterScope_;
    TypeRef *returnTypeRef_;
    gc_vector<ParameterDef*> parameters_;
    ExprStmt* body_;
    type::FunctionType *functionType_;

public:
    FuncDefStmt(
        source::SourceSpan sourceSpan,
        const std::string &name,
        TypeRef* returnTypeRef,
        gc_vector<ParameterDef*> parameters,
        ExprStmt* body
    ) : ExprStmt(sourceSpan),
        name_{name},
        parameterScope_{scope::StorageKind::Argument},
        returnTypeRef_{returnTypeRef},
        parameters_{parameters},
        body_{body},
        functionType_{createFunctionType(returnTypeRef->type(), parameters)}
    {
        parameterScope_.name() = name_ + "-parameters";
    }

    type::Type *type() const override { return &type::Primitives::Void; }
    bool canWrite() const override { return false; }

    std::string name() const { return name_; }
    type::Type *returnType() const { return functionType_->returnType(); }
    type::FunctionType *functionType() { return functionType_; }
    scope::SymbolTable *parameterScope() { return &parameterScope_; };
    ExprStmt *body() const { return body_; }
    void setBody(ExprStmt *body) { body_ = body; }

    scope::FunctionSymbol *symbol() const {
        ASSERT(symbol_ && "Function symbol not resolved yet")
        return symbol_;
    }

    void setSymbol(scope::FunctionSymbol *symbol) { symbol_ = symbol; }

    gc_vector<ParameterDef*> parameters() {
        gc_vector<ParameterDef*> retval;
        retval.reserve(parameters_.size());
        for(auto pd : parameters_) {
            retval.push_back(pd);
        }

        return retval;
    }

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingFuncDefStmt(this);
        if(visitor->shouldVisitChildren()) {
            returnTypeRef_->accept(visitor);
            for(auto p : parameters_) {
                p->accept(visitor);
            }
            body_->accept(visitor);
        }
        visitor->visitedFuncDeclStmt(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        gc_vector<ParameterDef*> clonedParameters;
        for(auto p : parameters_) {
            clonedParameters.push_back(p->deepCopy());
        }
        return new FuncDefStmt(sourceSpan_,
                               name_,
                               returnTypeRef_->deepCopyForTemplate(),
                               clonedParameters,
                               body_->deepCopyExpandTemplate(templateArgs));
    }
};

class MethodRefExpr : public ExprStmt {
    std::string name_;
    scope::FunctionSymbol *symbol_ = nullptr;
public:
    explicit MethodRefExpr(source::SourceSpan sourceSpan, const std::string &name) : ExprStmt(sourceSpan), name_{ name } {
        ASSERT(name.size() > 0);
    }

    virtual type::Type *type() const override {
        ASSERT(symbol_);
        return symbol_->type();
    }

    virtual std::string name() const { return name_; }
    std::string toString() const { return name_ + ":" + this->type()->nameForDisplay(); }

    scope::FunctionSymbol *symbol() { return symbol_; }
    void setSymbol(scope::FunctionSymbol *symbol) {
        symbol_ = symbol;
    }

    bool canWrite() const override { return true; };

    void accept(AstVisitor *visitor) override {
        visitor->visitMethodRefExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &) const override {
        return new MethodRefExpr(sourceSpan_, name_);
    }
};

class FuncCallExpr : public ExprStmt {
    ExprStmt *instanceExpr_;
    source::SourceSpan openParenSpan_;
    ExprStmt *funcExpr_;
    gc_vector<ExprStmt*> arguments_;
public:
    FuncCallExpr(
        const source::SourceSpan &span,
        ExprStmt *instanceExpr,
        const source::SourceSpan &openParenSpan,
        ast::ExprStmt *funcExpr,
        const gc_vector<ExprStmt*> &arguments
    ) : ExprStmt(span),
        instanceExpr_{instanceExpr},
        openParenSpan_{openParenSpan},
        funcExpr_{funcExpr},
        arguments_{arguments} { }

    ExprStmt *funcExpr() const { return funcExpr_; }
    ExprStmt *instanceExpr() const { return instanceExpr_; }

    bool canWrite() const override { return false; };

    type::Type *type() const override {
        auto functionType = dynamic_cast<type::FunctionType*>(funcExpr_->type());
        ASSERT(functionType && "Expression is not a function");
        return functionType->returnType();
    }

    gc_vector<ExprStmt*> arguments() const {
        gc_vector<ExprStmt *> args;
        args.reserve(arguments_.size());
        for (auto argument : arguments_) {
            args.push_back(argument);
        }
        return args;
    }

    void replaceArgument(size_t index, ExprStmt *newExpr) {
        ASSERT(index < arguments_.size());
        arguments_[index] = newExpr;
    }

    size_t argCount() { return arguments_.size(); }

    void accept(AstVisitor *visitor) override {
        visitor->visitingFuncCallExpr(this);
        if(visitor->shouldVisitChildren()) {
            funcExpr_->accept(visitor);
            if(instanceExpr_) {
                instanceExpr_->accept(visitor);
            }
            for (auto argument : arguments_) {
                argument->accept(visitor);
            }
        }
        visitor->visitedFuncCallExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        gc_vector<ExprStmt*> clonedArguments;
        clonedArguments.reserve(arguments_.size());
        for(auto a : arguments_) {
            clonedArguments.push_back(a->deepCopyExpandTemplate(templateArgs));
        }
        return new FuncCallExpr(sourceSpan_,
                                instanceExpr_->deepCopyExpandTemplate(templateArgs),
                                openParenSpan_,
                                funcExpr_->deepCopyExpandTemplate(templateArgs),
                                clonedArguments);
    }
};

class ClassDefinitionBase : public ExprStmt {
    std::string name_;
    CompoundExpr *body_;
protected:
    ClassDefinitionBase(
        source::SourceSpan span,
        const std::string &name,
        ast::CompoundExpr *body
    ) : ExprStmt{span},
        name_{name},
        body_{body}
    { }
public:

    /** The class definition, as an expression in the language, doesn't return a meaningful value. */
    type::Type *type() const override { return &type::Primitives::Void; }

    /** The type of the class being defined. */
    virtual type::Type *definedType() const = 0;

    bool canWrite() const override { return false; }

    CompoundExpr *body() const { return body_; }
    std::string name() const { return name_; }
};

inline gc_vector<type::Type*> toVectorOfType(const gc_vector<TemplateArgument*> &arguments) {
    gc_vector<type::Type*> vectorOfTypes;
    vectorOfTypes.reserve(arguments.size());
    for(auto arg : arguments) {
        vectorOfTypes.push_back(arg->typeRef()->type());
    }
    return vectorOfTypes;
}

class CompleteClassDefinition : public ClassDefinitionBase {
    gc_vector<TemplateArgument*> templateArguments_;
    type::Type *definedType_;

public:
    CompleteClassDefinition(
        source::SourceSpan span,
        const std::string &name,
        ast::CompoundExpr *body
    ) : ClassDefinitionBase{span, name, body},
        definedType_{new type::ClassType(AstNode::nodeId(), name, toVectorOfType(templateArguments_))}
    { }

    CompleteClassDefinition(
        source::SourceSpan span,
        const std::string &name,
        const gc_vector<TemplateArgument*> templateArgs,
        ast::CompoundExpr *body
    ) : ClassDefinitionBase{span, name, body},
        templateArguments_{templateArgs},
        definedType_{new type::ClassType(AstNode::nodeId(), name, toVectorOfType(templateArguments_))}
    { }

    /** The type of the class being defined. */
    type::Type *definedType() const override { return definedType_; }

    bool hasTemplateArguments() { return !templateArguments_.empty(); }

    void populateClassType() {
        auto ct = dynamic_cast<type::ClassType*>(definedType_);
        //TODO: the assert below would seem to indicate that we can split ClassDefinition into two types:
        //one for generic classes and one for complete classes.  From the class meant for generic types, we would have to return
        //new instances of the class meant for complete types from deepCopyExpandTemplate()
        ASSERT(ct);

        for (auto variable : this->body()->scope()->variables()) {
            ct->addField(variable->name(), variable->type());
        }

        for (auto method : this->body()->scope()->functions()) {
            method->setThisSymbol(new scope::VariableSymbol("this", this->definedType()));
            ct->addMethod(method->name(), method);
        }
    }

    void accept(AstVisitor *visitor) override {
        visitor->visitingCompleteClassDefinition(this);
        if (visitor->shouldVisitChildren()) {
            body()->accept(visitor);
        }
        visitor->visitedCompleteClassDefinition(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        //Only once inside an expanded template, a generic class not generic anymore.
        auto *completeClassDef = new CompleteClassDefinition(
            sourceSpan_,
            name(),
            templateArguments_,
            static_cast<CompoundExpr*>(body()->deepCopyExpandTemplate(templateArgs)));

        return completeClassDef;
    }
};

class GenericClassDefinition : public ClassDefinitionBase {
    gc_vector<ast::TemplateParameter*> templateParameters_;
    type::GenericType *definedType_;

    inline static std::vector<std::string> getTemplateParameterNames(const gc_vector<TemplateParameter*> &parameters) {
        std::vector<std::string> names;
        for(auto parameter : parameters) {
            names.push_back(parameter->name());
        }
        return names;
    }

public:
    GenericClassDefinition(
        source::SourceSpan span,
        const std::string &name,
        const gc_vector<TemplateParameter*> &templateParameters,
        ast::CompoundExpr *body
    ) : ClassDefinitionBase{span, name, body},
        templateParameters_{templateParameters},
        definedType_{new type::GenericType(nodeId(), name, getTemplateParameterNames(templateParameters))}
    { }

    /** The type of the class being defined. */
    type::Type *definedType() const override { return definedType_; }


    void accept(AstVisitor *visitor) override {
        visitor->visitingGenericClassDefinition(this);
        if (visitor->shouldVisitChildren()) {
            body()->accept(visitor);
        }
        visitor->visitedGenericClassDefinition(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        //Only once inside an expanded template, a generic class is not generic anymore so construct CompleteClassDefinition
        auto *completeClassDef = new CompleteClassDefinition(
            sourceSpan_,
            name(),
            templateArgs,
            static_cast<CompoundExpr*>(body()->deepCopyExpandTemplate(templateArgs)));

        upcast<type::ClassType>(completeClassDef->definedType())->setGenericType(definedType_);

        return completeClassDef;
    }
};


class DotExpr : public ExprStmt {
    source::SourceSpan dotSourceSpan_;
    ExprStmt *lValue_;
    std::string memberName_;
    type::ClassField *field_ = nullptr;
    bool isWrite_ = false;
public:
    DotExpr(const source::SourceSpan &sourceSpan,
            const source::SourceSpan &dotSourceSpan,
            ExprStmt *lValue,
            const std::string &memberName)
        : ExprStmt(sourceSpan), dotSourceSpan_{dotSourceSpan}, lValue_{lValue}, memberName_{memberName} { }

    source::SourceSpan dotSourceSpan() { return dotSourceSpan_; };

    bool canWrite() const override { return true; };

    bool isWrite() { return isWrite_; }
    void setIsWrite(bool isWrite) {
        //Eventually, this will need to propagate to the deepest DotExpr, not the first one that's invoked...
        isWrite_ = isWrite;
    }

    ExprStmt *lValue() { return lValue_; }
    std::string memberName() { return memberName_; }

    type::ClassField *field() { return field_; }
    void setField(type::ClassField *field) { field_ = field; }

    type::Type *type() const override {
        ASSERT(field_ && "Field must be resolved first");
        return field_->type();
    }

    void accept(AstVisitor *visitor) override {
        visitor->visitingDotExpr(this);
        if (visitor->shouldVisitChildren()) {
            lValue_->accept(visitor);
        }
        visitor->visitedDotExpr(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new DotExpr(sourceSpan_, dotSourceSpan_, lValue_->deepCopyExpandTemplate(templateArgs), memberName_);
    }
};

class AssertExprStmt : public ExprStmt {
    ast::ExprStmt *condition_;
public:
    AssertExprStmt(const source::SourceSpan &sourceSpan, ExprStmt *condition)
        : ExprStmt(sourceSpan), condition_{condition} { }

    type::Type *type() const override { return &type::Primitives::Void; }
    bool canWrite() const override { return false; }
    ast::ExprStmt *condition() { return condition_; }
    void setCondition(ast::ExprStmt *condition) { condition_ = condition; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingAssertExprStmt(this);
        if(visitor->shouldVisitChildren()) {
            condition_->accept(visitor);
        }
        visitor->visitedAssertExprStmt(this);
    }

    ExprStmt *deepCopyExpandTemplate(const TemplateArgVector &templateArgs) const override {
        return new AssertExprStmt(sourceSpan_, condition_->deepCopyExpandTemplate(templateArgs));
    }
};


class Module : public AstNode {
    std::string name_;
    CompoundExpr *body_;
    gc_unordered_map<std::string, TemplateExprStmt*> templates_;
public:
    Module(const std::string &name, CompoundExpr* body)
        : name_{name}, body_{body} {
    }


    void addTemplate(TemplateExprStmt *templ) {
        templates_[templ->name()] = templ;
    }

    TemplateExprStmt *findTemplate(const std::string &name) {
        return templates_[name];
    }

    std::string name() const { return name_; }

    scope::SymbolTable *scope() { return body_->scope(); }

    CompoundExpr *body() { return body_; }

    void accept(AstVisitor *visitor) override {
        visitor->visitingModule(this);
        if(visitor->shouldVisitChildren()) {
            body_->accept(visitor);
        }
        visitor->visitedModule(this);
    }
};

class AnodeWorld : public gc, no_assign, no_copy {
    scope::SymbolTable globalScope_{scope::StorageKind::Global, ""};
    gc_unordered_map<UniqueId, TemplateExprStmt*> templateIndex_;

public:

    scope::SymbolTable *globalScope() { return &globalScope_; }

    void addTemplate(TemplateExprStmt *templateExprStmt) {
        templateIndex_[templateExprStmt->nodeId()] = templateExprStmt;
    }

    TemplateExprStmt *getTemplate(UniqueId nodeId) {
        return templateIndex_[nodeId];
    }

};

}}}
