#pragma once
#pragma once

#include "lwnn.h"

#include "type.h"
#include "scope.h"
#include "source.h"

#include <string>
#include <functional>
#include <memory>

namespace lwnn { namespace ast {

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
class ReturnStmt;
class ExprStmt;
class CompoundExpr;
class ParameterDef;
class FuncDefStmt;
class FuncCallExpr;
class ClassDefinition;
class ReturnStmt;
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
class TypeRef;
class AssertExprStmt;
class Module;

class AstVisitor : public gc {
public:
    //////////// Statements
    /** Executes before every Stmt is visited. */
    virtual bool visitingStmt(Stmt *) { return true; }
    /** Executes after every Stmt is visited. */
    virtual void visitedStmt(Stmt *) { }

    virtual void visitingParameterDef(ParameterDef *) { }
    virtual void visitedParameterDef(ParameterDef *) { }

    virtual bool visitingFuncDefStmt(FuncDefStmt *) { return true; }
    virtual void visitedFuncDeclStmt(FuncDefStmt *) { }

    virtual void visitingFuncCallExpr(FuncCallExpr *) {  }
    virtual void visitedFuncCallExpr(FuncCallExpr *) { }

    virtual bool visitingClassDefinition(ClassDefinition *) { return true; }
    virtual void visitedClassDefinition(ClassDefinition *) {  }

    virtual void visitingReturnStmt(ReturnStmt *) { }
    virtual void visitedReturnStmt(ReturnStmt *) { }

    //////////// Expressions
    /** Executes before every ExprStmt is visited. Return false to prevent visitation of the node's descendants. */
    virtual bool visitingExprStmt(ExprStmt *) { return true; }
    /** Executes after every ExprStmt is visited. */
    virtual void visitedExprStmt(ExprStmt *) { }

    virtual bool visitingAssertExprStmt(AssertExprStmt *) { return true; }
    virtual void visitedAssertExprStmt(AssertExprStmt *) { }

    virtual void visitingVariableDeclExpr(VariableDeclExpr *) { }
    virtual void visitedVariableDeclExpr(VariableDeclExpr *) { }

    virtual bool visitingIfExpr(IfExprStmt *) { return true; }
    virtual void visitedIfExpr(IfExprStmt *) { }

    virtual bool visitingWhileExpr(WhileExpr *) { return true; }
    virtual void visitedWhileExpr(WhileExpr *) { }

    virtual bool visitingBinaryExpr(BinaryExpr *) { return true; }
    virtual void visitedBinaryExpr(BinaryExpr *) { }

    virtual bool visitingUnaryExpr(UnaryExpr *) { return true; }
    virtual void visitedUnaryExpr(UnaryExpr *) { }

    virtual void visitLiteralBoolExpr(LiteralBoolExpr *) { }

    virtual void visitLiteralInt32Expr(LiteralInt32Expr *) { }

    virtual void visitLiteralFloatExpr(LiteralFloatExpr *) { }

    virtual void visitVariableRefExpr(VariableRefExpr *) { }

    virtual void visitingCastExpr(CastExpr *) { }
    virtual void visitedCastExpr(CastExpr *) { }

    virtual void visitingDotExpr(DotExpr *) { }
    virtual void visitedDotExpr(DotExpr *) { }

    virtual bool visitingCompoundExpr(CompoundExpr *) { return true; }
    virtual void visitedCompoundExpr(CompoundExpr *) { }

    //////////// Misc
    virtual void visitTypeRef(TypeRef *) { }

    virtual bool visitingModule(Module *) { return true; }
    virtual void visitedModule(Module *) { }

};

extern unsigned long astNodesDestroyedCount;

/** Base class for all nodes */
//class AstNode : public gc_cleanup, no_copy, no_assign {
class AstNode : public gc , no_copy, no_assign {
public:
    virtual ~AstNode() {
        astNodesDestroyedCount++;
    }
    virtual void accept(AstVisitor *visitor) = 0;
};

template<typename TAstNode>
bool isInstanceOf(AstNode *node) {
    return dynamic_cast<TAstNode*>(node);
}

template<typename TAstNode>
TAstNode *upcast(AstNode *node) {
    TAstNode *upcasted = dynamic_cast<TAstNode*>(node);
    if(!upcasted){
        ASSERT_FAIL("Attempted to perform an invalid upcast");
    }

    return upcasted;
}


/** A statement any kind of non-terminal that may appear within a CompountStatement (i.e. between { and }), or
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

/* Refers to a data type, i.e. "int", or "WidgetFactory."
 * Initially, type references will be unresolved (i.e. referencedType_ is null) but this is resolved
 * during an AST pass. */
class TypeRef : public AstNode {
    source::SourceSpan sourceSpan_;
    const std::string name_;
    type::ResolutionDeferredType *referencedType_;

public:
    /** Constructor to be used when the data type isn't known yet and needs to be resolved later. */
    TypeRef(const source::SourceSpan &sourceSpan, std::string name)
        : sourceSpan_(sourceSpan), name_(name), referencedType_(new type::ResolutionDeferredType()) { }

    /** Constructor to be used when the data type is known and doesn't need to be resolved. */
    TypeRef(source::SourceSpan sourceSpan, type::Type *dataType)
        : sourceSpan_(sourceSpan), name_(dataType->name()), referencedType_(new type::ResolutionDeferredType(dataType)) { }

    source::SourceSpan sourceSpan() {
        return sourceSpan_;
    }

    std::string name() const { return name_; }

    type::Type *type() const {
        ASSERT(referencedType_ && "Data type hasn't been resolved yet");
        return referencedType_;
    }

    void setType(type::Type *referencedType) {
        referencedType_->resolve(referencedType);
    }
    virtual void accept(AstVisitor *visitor) override {
        visitor->visitTypeRef(this);
    }
};




/** Base class for all expressions. */
class ExprStmt : public Stmt {
protected:
    ExprStmt(const source::SourceSpan &sourceSpan) : Stmt(sourceSpan) { }
public:
    virtual ~ExprStmt() { }
    virtual type::Type *type() const  = 0;
    virtual bool canWrite() const = 0;
};

/** Represents a literal boolean */
class LiteralBoolExpr : public ExprStmt {
    bool const value_;
public:
    LiteralBoolExpr(source::SourceSpan sourceSpan, const bool value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ~LiteralBoolExpr() {}
    //virtual ExprKind exprKind() const override { return ExprKind::LiteralBoolExpr; }
    type::Type *type() const override { return &type::Primitives::Bool; }
    bool value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExprStmt(this);
        visitor->visitLiteralBoolExpr(this);
        visitor->visitedExprStmt(this);
    }
};

/** Represents an expression that is a literal 32 bit integer. */
class LiteralInt32Expr : public ExprStmt {
    int const value_;
public:
    LiteralInt32Expr(source::SourceSpan sourceSpan, const int value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ~LiteralInt32Expr() {}
    type::Type *type() const override { return &type::Primitives::Int32; }
    int value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExprStmt(this);
        visitor->visitLiteralInt32Expr(this);
        visitor->visitedExprStmt(this);
    }
};

/** Represents an expression that is a literal float. */
class LiteralFloatExpr : public ExprStmt {
    float const value_;
public:
    LiteralFloatExpr(source::SourceSpan sourceSpan, const float value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ~LiteralFloatExpr() {}
    //ExprKind exprKind() const override { return ExprKind::LiteralFloatExpr; }
    type::Type *type() const override {  return &type::Primitives::Float; }
    float value() const { return value_; }

    virtual bool canWrite() const override { return false; };


    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExprStmt(this);
        visitor->visitLiteralFloatExpr(this);
        visitor->visitedExprStmt(this);
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

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingUnaryExpr(this) ? visitChildren : false;

        if(visitChildren) {
            valueExpr_->accept(visitor);
        }
        visitor->visitedUnaryExpr(this);
        visitor->visitedExprStmt(this);
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
    virtual ~BinaryExpr() { }

    source::SourceSpan operatorSpan() { return operatorSpan_; }

    //ExprKind exprKind() const override { return ExprKind::BinaryExpr; }

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
                ASSERT_FAIL("Unknown binary operation kind")
        }
    }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingBinaryExpr(this) ? visitChildren : false;

        if(visitChildren) {
            lValue_->accept(visitor);
            rValue_->accept(visitor);
        }
        visitor->visitedBinaryExpr(this);
        visitor->visitedExprStmt(this);
    }
};

enum class VariableAccess : unsigned char {
    Read,
    Write
};

/** Represents a reference to a previously declared variable. */
class VariableRefExpr : public ExprStmt {
    std::string name_;
    scope::Symbol *symbol_ = nullptr;
    VariableAccess access_ = VariableAccess::Read;
public:
    VariableRefExpr(source::SourceSpan sourceSpan, const std::string &name) : ExprStmt(sourceSpan), name_{ name } {
        ASSERT(name.size() > 0);
    }
    //ExprKind exprKind() const override { return ExprKind::VariableRefExpr; }

    virtual type::Type *type() const override {
        ASSERT(symbol_);
        return symbol_->type();
    }

    virtual std::string name() const { return name_; }
    std::string toString() const { return name_ + ":" + this->type()->name(); }

    scope::Symbol *symbol() { return symbol_; }
    void setSymbol(scope::Symbol *symbol) {
        symbol_ = symbol;
    }

    virtual bool canWrite() const override { return true; };

    VariableAccess variableAccess() { return access_; };
    void setVariableAccess(VariableAccess access) { access_ = access; }

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExprStmt(this);
        visitor->visitVariableRefExpr(this);
        visitor->visitedExprStmt(this);
    }
};


/** Defines a variable and references it. */
class VariableDeclExpr : public VariableRefExpr {
    TypeRef* typeRef_;
public:
    VariableDeclExpr(source::SourceSpan sourceSpan, const std::string &name, TypeRef* typeRef)
        : VariableRefExpr(sourceSpan, name),
          typeRef_(typeRef)
    {
    }

    virtual std::string name() const override { return VariableRefExpr::name(); }

    //ExprKind exprKind() const override { return ExprKind::VariableDeclExpr; }
    TypeRef *typeRef() { return typeRef_; }
    virtual type::Type *type() const override { return typeRef_->type(); }

    //virtual std::string toString() const override {  return this->name() + ":" + typeRef_->name(); }

    virtual bool canWrite() const override { return true; };

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitor->visitingVariableDeclExpr(this);

        if(visitChildren) {
            typeRef_->accept(visitor);
        }
        visitor->visitedVariableDeclExpr(this);
        visitor->visitedExprStmt(this);
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
        : ExprStmt(sourceSpan), toType_(new TypeRef(sourceSpan, toType)), valueExpr_(valueExpr), castKind_(castKind) { }

    /** Use this constructor when the type::Type of the cast is *not* known in advance. */
    CastExpr(source::SourceSpan sourceSpan, TypeRef *toType, ExprStmt *valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toType_(toType), valueExpr_(valueExpr), castKind_(castKind) { }

    static CastExpr *createImplicit(ExprStmt *valueExpr, type::Type *toType);

    //ExprKind exprKind() const override { return ExprKind::CastExpr; }
    type::Type *type() const  override{ return toType_->type(); }
    CastKind castKind() const { return castKind_; }

    virtual bool canWrite() const override { return false; };

    ExprStmt *valueExpr() const { return valueExpr_; }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitor->visitingCastExpr(this);

        if(visitChildren) {
            toType_->accept(visitor);
            valueExpr_->accept(visitor);
        }

        visitor->visitedCastExpr(this);
        visitor->visitedExprStmt(this);
    }
};

/** Contains a series of expressions, i.e. those contained within { and }. */
class CompoundExpr : public ExprStmt {
    scope::SymbolTable scope_;
protected:
    gc_vector<ExprStmt*> expressions_;
public:
    CompoundExpr(source::SourceSpan sourceSpan, scope::StorageKind storageKind) : ExprStmt(sourceSpan), scope_{storageKind} { }
    CompoundExpr(source::SourceSpan sourceSpan, scope::StorageKind storageKind, const gc_vector<ExprStmt*> expressions)
        : ExprStmt(sourceSpan), scope_{storageKind}, expressions_{expressions} { }

    virtual ~CompoundExpr() {}
    scope::SymbolTable *scope() { return &scope_; }

    //virtual ExprKind exprKind() const { return ExprKind::CompoundExpr; };
    virtual type::Type *type() const {
        ASSERT(expressions_.size() > 0);
        return expressions_.back()->type();
    };
    virtual bool canWrite() const { return false; };

    void addExpr(ExprStmt* stmt) {
        expressions_.push_back(stmt);
    }

    gc_vector<ExprStmt*> expressions() const {
        gc_vector<ExprStmt*> retval;
        retval.reserve(expressions_.size());
        for(auto &stmt : expressions_) {
            retval.push_back(stmt);
        }
        return retval;
    }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingCompoundExpr(this) ? visitChildren : false;
        if(visitChildren) {
            for (auto &stmt : expressions_) {
                stmt->accept(visitor);
            }
        }
        visitor->visitedCompoundExpr(this);
        visitor->visitedExprStmt(this);
    }
};

/** Represents a return statement.  */
class ReturnStmt : public Stmt {
    ExprStmt* valueExpr_;
public:
    ReturnStmt(source::SourceSpan sourceSpan, ExprStmt* valueExpr)
        : Stmt(sourceSpan), valueExpr_(valueExpr) {}

    //StmtKind stmtKind() const override { return StmtKind::ReturnStmt; }
    const ExprStmt *valueExpr() const { return valueExpr_; }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingStmt(this);
        visitor->visitingReturnStmt(this);
        if(visitChildren) {
            valueExpr_->accept(visitor);
        }
        visitor->visitedReturnStmt(this);
        visitor->visitedStmt(this);
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

    //virtual ExprKind exprKind() const override {  return ExprKind::ConditionalExpr; }

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

    virtual bool canWrite() const override { return false; };

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingIfExpr(this) ? visitChildren : false;

        if(visitChildren) {
            condition_->accept(visitor);
            thenExpr_->accept(visitor);
            if(elseExpr_)
                elseExpr_->accept(visitor);
        }

        visitor->visitedIfExpr(this);
        visitor->visitedExprStmt(this);
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

    //virtual ExprKind exprKind() const override {  return ExprKind::ConditionalExpr; }

    type::Type *type() const override {
        //For now, while expressions will not return a value.
        return &type::Primitives::Void;
    }

    ExprStmt *condition() const { return condition_; }

    void setCondition(ExprStmt *newCondition) { condition_ = newCondition; }

    ExprStmt *body() const { return body_; }

    virtual bool canWrite() const override { return false; };

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingWhileExpr(this) ? visitChildren : false;

        if(visitChildren) {
            condition_->accept(visitor);
            body_->accept(visitor);
        }

        visitor->visitedWhileExpr(this);
        visitor->visitedExprStmt(this);
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

    source::SourceSpan span() { return span(); }
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
        typeRef_->accept(visitor);
        visitor->visitedParameterDef(this);
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
        std::string name,
        TypeRef* returnTypeRef,
        gc_vector<ParameterDef*> parameters,
        ExprStmt* body
    ) : ExprStmt(sourceSpan),
        name_{name},
        parameterScope_{scope::StorageKind::Argument},
        returnTypeRef_{returnTypeRef},
        parameters_{parameters},
        body_{body},
        functionType_{createFunctionType(returnTypeRef->type(), parameters)} { }

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
        bool visitChildren = visitor->visitingStmt(this);
        visitChildren = visitor->visitingFuncDefStmt(this) ? visitChildren : false;
        if(visitChildren) {
            returnTypeRef_->accept(visitor);
            for(auto p : parameters_) {
                p->accept(visitor);
            }
            body_->accept(visitor);
        }
        visitor->visitedFuncDeclStmt(this);
        visitor->visitedStmt(this);
    }
};


class FuncCallExpr : public ExprStmt {
    source::SourceSpan openParenSpan_;
    ExprStmt *funcExpr_;
    gc_vector<ExprStmt*> arguments_;
public:
    FuncCallExpr(
        const source::SourceSpan &span,
        const source::SourceSpan &openParenSpan,
        ast::ExprStmt *funcExpr,
        const gc_vector<ExprStmt*> &arguments
    ) : ExprStmt(span),
        openParenSpan_{openParenSpan},
        funcExpr_{funcExpr},
        arguments_{arguments} { }
    ExprStmt *funcExpr() { return funcExpr_; }

    //virtual ExprKind exprKind() const override { return ExprKind::FuncCallExpr; };
    virtual bool canWrite() const override { return false; };

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

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitor->visitingFuncCallExpr(this);
        if(visitChildren) {
            funcExpr_->accept(visitor);
            for (auto argument : arguments_) {
                argument->accept(visitor);
            }
        }
        visitor->visitedFuncCallExpr(this);
        visitor->visitedExprStmt(this);
    }
};



class ClassDefinition : public ExprStmt {
    std::string name_;
    CompoundExpr *body_;

    type::ClassType *classType_;

    inline static CompoundExpr *possiblyWrapExprInCompoundExpr(ast::ExprStmt *expr) {
        CompoundExpr *compoundExpr = dynamic_cast<CompoundExpr*>(expr);
        if(compoundExpr) return compoundExpr;
        compoundExpr = new CompoundExpr(expr->sourceSpan(), scope::StorageKind::Instance);
        compoundExpr->addExpr(expr);
        return compoundExpr;
    }

public:
    ClassDefinition(source::SourceSpan span, std::string name, ast::ExprStmt *body)
        : ExprStmt{span},
          name_{name},
          body_{possiblyWrapExprInCompoundExpr(body)},
          classType_{new type::ClassType(name)} { }

    type::Type *type() const override { return &type::Primitives::Void; }
    bool canWrite() const override { return false; }

    CompoundExpr *body() { return body_; }

    std::string name() { return name_; }

    type::ClassType *classType() { return classType_; }

    void populateClassType() {
        for(auto variable : this->body()->scope()->variables()) {
            classType_->addField(variable->name(), variable->type());
        }
    }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingStmt(this);
        visitChildren = visitor->visitingClassDefinition(this) ? visitChildren : false;
        if(visitChildren) {
            body_->accept(visitor);
        }
        visitor->visitedClassDefinition(this);
        visitor->visitedStmt(this);
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

    //virtual ExprKind exprKind() const override { return ExprKind::DotExpr; };
    virtual bool canWrite() const override { return true; };

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

    virtual void accept(AstVisitor *visitor) override {
        visitor->visitingExprStmt(this);
        visitor->visitingDotExpr(this);
        lValue_->accept(visitor);
        visitor->visitedDotExpr(this);
        visitor->visitedExprStmt(this);
    }
};

class AssertExprStmt : public ExprStmt {
    ast::ExprStmt *condition_;
public:
    AssertExprStmt(const source::SourceSpan &sourceSpan, ExprStmt *condition)
        : ExprStmt(sourceSpan), condition_{condition} { }

    virtual type::Type *type() const { return &type::Primitives::Void; }
    virtual bool canWrite() const { return false; };


    ast::ExprStmt *condition() { return condition_; }
    void setCondition(ast::ExprStmt *condition) {
        condition_ = condition;
    }

    virtual void accept(AstVisitor *visitor) override {
        bool visitChildren = visitor->visitingExprStmt(this);
        visitChildren = visitor->visitingAssertExprStmt(this) ? visitChildren : false;
        if(visitChildren) {
            condition_->accept(visitor);
        }
        visitor->visitedAssertExprStmt(this);
        visitor->visitedExprStmt(this);
    }
};

class Module : public AstNode {
    std::string name_;
    CompoundExpr *body_;
public:
    Module(const std::string &name, CompoundExpr* body)
        : name_{name}, body_{body} {
    }

    std::string name() const { return name_; }

    scope::SymbolTable *scope() { return body_->scope(); }

    CompoundExpr *body() { return body_; }

    void accept(AstVisitor *visitor) override {
        if(visitor->visitingModule(this)) {
            body_->accept(visitor);
        }
        visitor->visitedModule(this);
    }
 };

}}



