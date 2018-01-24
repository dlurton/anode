#pragma once

#include "anode.h"
#include "front/unique_id.h"
#include "front/type.h"
#include "front/scope.h"
#include "front/source.h"

#include <string>
#include <functional>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>

#include "AstVisitor.h"

namespace anode { namespace front { namespace ast {


extern unsigned long astNodesDestroyedCount;


class Identifier {
    source::SourceSpan span_;
    std::string text_;

public:
    Identifier(const Identifier &) = default;
    Identifier(source::SourceSpan span, const std::string &text)
        : span_(span), text_(text) { }

    const std::string &text() const { return text_; }
    const source::SourceSpan &span() const { return span_; }

};


/** Represents a multi-part identifier, i.e. "system::io::filesystem" */
class MultiPartIdentifier {
    const source::SourceSpan span_;
    const std::vector<Identifier> parts_;

    inline static source::SourceSpan spanFromIdVector(const std::vector<Identifier> &parts) {
        const source::SourceSpan &firstSpan = parts.front().span();
        const source::SourceSpan &lastSpan = parts.back().span();
        return source::SourceSpan(firstSpan.name(), firstSpan.start(), lastSpan.end());
    }

    inline static std::vector<Identifier> idVectorFromId(const Identifier &part) {
        std::vector<Identifier> parts;
        parts.push_back(part);
        return parts;
    }

public:
    MultiPartIdentifier(const std::vector<Identifier> &parts) : span_{spanFromIdVector(parts)}, parts_{parts} {
        ASSERT(parts_.size() >= 1);
    }
    MultiPartIdentifier(const Identifier part) : span_{part.span()}, parts_{idVectorFromId(part)} {
        ASSERT(parts_.size() >= 1);
    }

    const source::SourceSpan &span() const { return span_; }

    int size() const { return (int)parts_.size(); }

    std::vector<Identifier> allParts() const {
        return parts_;
    }

    /** The first identifier element. */
    const Identifier &front() const {
        return parts_.front();
    }

    typedef std::vector<Identifier>::const_iterator part_iterator;

    part_iterator begin() const { return parts_.begin(); }
    part_iterator end() const { return parts_.end(); }

    typedef std::vector<std::reference_wrapper<const Identifier>> middle_vector;
    /** Everything between but not including the first and last identifier elements. */
    middle_vector middle() const {
        middle_vector middle;
        if(parts_.size() > 2) {
            std::copy(parts_.begin() + 1, parts_.end() - 1, std::back_inserter(middle));
        }
        return middle;
    }

    /** The last identiufier element. */
    const Identifier &back() const {
        return parts_.back();
    }

    std::string qualifedName() const {
        return string::join<std::vector<Identifier>::const_iterator, Identifier>(
            parts_.begin(), parts_.end(), "::", [](const Identifier &id) { return id.text(); });
    }
};

/** Base class for all nodes */
class AstNode : public Object {
    UniqueId nodeId_;
    const source::SourceSpan sourceSpan_;
    
    AstNode *parent_ = nullptr;
    
protected:

    /** This method should *never* be invoked directly--only thru acceptVisitor(). */
    virtual void accept(AstVisitor &visitor) = 0;
public:
    //NO_COPY_NO_ASSIGN(AstNode)
    AstNode(const source::SourceSpan &span);
    AstNode(const AstNode &n) : AstNode(n.sourceSpan_){ }

    virtual ~AstNode() {
        astNodesDestroyedCount++;
    }
    
    virtual AstNodeKind astNodeKind() = 0;
    
    const source::SourceSpan &sourceSpan() const {
        return sourceSpan_;
    }
    
    UniqueId nodeId() const { return nodeId_; }

    AstNode &parent() const {
        ASSERT(parent_);
        return *parent_;
    };

    void setParent(AstNode &parent) {
        parent_ = &parent;
    }

    void acceptVisitor(AstVisitor &visitor) {
        visitor.beforeAccept(*this);
        accept(visitor);
        visitor.afterAccept(*this);
    }
};


/** AST node representing a reference to a data type. */
class TypeRef : public AstNode {
protected:
    TypeRef(const source::SourceSpan &sourceSpan) : AstNode{sourceSpan} { }
public:
    virtual type::Type &type() const = 0;
    virtual const MultiPartIdentifier &name() const = 0;
    virtual TypeRef &deepCopyForTemplate() const = 0;
    AstNodeKind astNodeKind() override { return AstNodeKind::TypeRef; }
    virtual TypeRefKind typeRefKind() = 0;
};


/** A reference to a data type that is known at parse time (used for primitive data types). */
class KnownTypeRef : public TypeRef {
    type::Type &referencedType_;
    const MultiPartIdentifier name_;
public:
    KnownTypeRef(source::SourceSpan sourceSpan, type::Type &dataType)
        : TypeRef(sourceSpan), referencedType_{dataType}, name_{
                MultiPartIdentifier(Identifier(source::SourceSpan::Any, referencedType_.name()))}
    { }
    TypeRefKind typeRefKind() override { return TypeRefKind::Known; }

    type::Type &type() const override { return referencedType_; };
    const MultiPartIdentifier &name() const override { return name_; };

    TypeRef &deepCopyForTemplate() const override {
        return *new KnownTypeRef(sourceSpan(), referencedType_);
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitKnownTypeRef(*this);
    }
};


/** A reference to a data type that doesn't exist at parse time. i.e.: classes.  Resolved during an AST pass. */
class ResolutionDeferredTypeRef : public TypeRef {
    const MultiPartIdentifier name_;
    gc_ref_vector<ResolutionDeferredTypeRef> templateArgs_;
    type::ResolutionDeferredType *referencedType_;

    inline static gc_ref_vector<type::Type> getTypesFromTypeRefs(const gc_ref_vector<ResolutionDeferredTypeRef> &typeRefs) {
        gc_ref_vector<type::Type> types;
        types.reserve(typeRefs.size());
        for(TypeRef &t : typeRefs) {
            types.emplace_back(t.type());
        }
        return types;
    }
public:
    ResolutionDeferredTypeRef(const source::SourceSpan &sourceSpan, const MultiPartIdentifier &name, const gc_ref_vector<ResolutionDeferredTypeRef> &args)
        : TypeRef(sourceSpan), name_{name}, templateArgs_{args}, referencedType_(new type::ResolutionDeferredType(getTypesFromTypeRefs(templateArgs_))) { }
    TypeRefKind typeRefKind() override { return TypeRefKind::ResolutionDeferred; }

    const MultiPartIdentifier &name() const override { return name_; }

    type::Type &type() const override {
         return *referencedType_;
    }

    bool isReferencingSameType(ast::ResolutionDeferredTypeRef &other) {
        if(this == &other) return true;
        ASSERT(isResolved() && other.isResolved());

    return referencedType_->isSameType(other.referencedType_);
}

    /** Convenience method so callers don't have to upcast return value of type() */
    type::ResolutionDeferredType *resolutionDeferredType() {
        ASSERT(referencedType_ && "ResolutionDeferredType type hasn't been resolved yet");
        return referencedType_;
    }

    bool isResolved() { return referencedType_->isResolved(); }

    bool hasTemplateArguments() { return !templateArgs_.empty(); }
    const gc_ref_vector<ResolutionDeferredTypeRef> &templateArgTypeRefs() { return templateArgs_; }
    const gc_ref_vector<type::Type> templateArgsTypes() { return getTypesFromTypeRefs(templateArgs_); }

    void setType(type::Type &referencedType) {
        referencedType_->resolve(&referencedType);
    }

    void accept(AstVisitor &visitor) override {
        for(ResolutionDeferredTypeRef &ta : templateArgs_) {
            ta.acceptVisitor(visitor);
        }
        visitor.visitedResolutionDeferredTypeRef(*this);
    }

    TypeRef& deepCopyForTemplate() const override {
        gc_ref_vector<ResolutionDeferredTypeRef> templateArgs;
        for(ResolutionDeferredTypeRef &ta : templateArgs_) {
            templateArgs.emplace_back(downcast<ResolutionDeferredTypeRef>(ta.deepCopyForTemplate()));
        }

        return *new ResolutionDeferredTypeRef(sourceSpan(), name_, templateArgs);
    }
};

template<typename TItem>
gc_ref_vector<TItem> deepCopyVector(const gc_ref_vector<TItem> copyFrom) {
    gc_ref_vector<TItem> copy;
    for(TItem &item : copyFrom) {
        copy.emplace_back(item.deepCopyForTemplate());
    }
    return copy;
}

/** Represents an instance of a template argument. */
class TemplateArgument {
    Identifier parameterName_;
    TypeRef &typeRef_;
public:
    TemplateArgument(const Identifier &parameterName, TypeRef &typeRef) : parameterName_{parameterName}, typeRef_{typeRef} { }
    const Identifier &parameterName() { return parameterName_; }
    TypeRef &typeRef() const { return typeRef_; }
};

typedef gc_ref_vector<TemplateArgument> TemplateArgVector;
class TemplateParameter : public AstNode {
    const Identifier name_;
public:
    TemplateParameter(const source::SourceSpan &sourceSpan, const Identifier &name) : AstNode(sourceSpan), name_{name} { }

    const Identifier &name() const { return name_; }
    AstNodeKind astNodeKind() override { return AstNodeKind::TemplateParameter; }

    void accept(AstVisitor &visitor) override {
        visitor.visitTemplateParameter(*this);
    }

    TemplateParameter &deepCopyForTemplate() const {
        return *new TemplateParameter(sourceSpan(), name());
    }
};

enum class ExpansionKind {
    AnonymousTemplate,
    NamedTemplate
};

/** Contains details about a template expansion. */
struct TemplateExpansionContext {
    const ExpansionKind expansionKind;
    const TemplateArgVector args;
    TemplateExpansionContext(ExpansionKind expansionKind, const TemplateArgVector &args) : expansionKind{expansionKind}, args{args} { }
};

/** Base class for all expressions. */
class ExprStmt : public AstNode {
protected:
    explicit ExprStmt(const source::SourceSpan &sourceSpan) : AstNode(sourceSpan) { }
    explicit ExprStmt(const ExprStmt &from) : AstNode(from) { }
public:
    ~ExprStmt() override = default;
    AstNodeKind astNodeKind() override { return AstNodeKind::ExprStmt; }
    virtual ExprStmtKind exprStmtKind() = 0;

    virtual type::Type &exprType() const  = 0;
    virtual bool canWrite() const = 0;
    virtual ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const = 0;
};

/** AST nodes representing language constructs which do not return a meaningful value (i.e. a function or definition, a while statemnt,
 * template definition or template expansion, etc, should inherit from this class. */
class VoidExprStmt : public ExprStmt {
protected:
    VoidExprStmt(const source::SourceSpan &sourceSpan) : ExprStmt(sourceSpan) { }
public:
    type::Type &exprType() const override { return type::ScalarType::Void; };
    bool canWrite() const override { return false; }
};

/** Like CompoundExpr but does not create a lexical scope. */
class ExpressionListExprStmt : public ExprStmt {
    gc_ref_vector<ExprStmt> expressions_;
public:
    ExpressionListExprStmt(source::SourceSpan sourceSpan, const gc_ref_vector<ExprStmt> &expressions)
        : ExprStmt(sourceSpan),
          expressions_{expressions}
    {
    }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::ExpressionList; }

    type::Type &exprType() const override {
        ASSERT(expressions_.size() > 0);
        return expressions_.back().get().exprType();
    };

    bool canWrite() const override { return false; };

    const gc_ref_vector<ExprStmt> &expressions() const {
        return expressions_;
    }

    void append(ExprStmt &exprStmt) {
        expressions_.emplace_back(exprStmt);
    }

    virtual void accept(AstVisitor &visitor) override {
        visitor.visitingExpressionList(*this);

        if(visitor.shouldVisitChildren()) {
            //Copy expressions_ because expressions can be added while visiting children
            //(adding items to vector invalidates iterator)
            gc_ref_vector<ExprStmt> exprs = expressions_;
            for (auto &&stmt : exprs) {
                stmt.get().acceptVisitor(visitor);
            }
        }
        visitor.visitedExpressionList(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ExprStmt> clonedExprs;
        clonedExprs.reserve(expressions_.size());
        for(ExprStmt &exprStmt : expressions_) {
            clonedExprs.emplace_back(exprStmt.deepCopyExpandTemplate(expansionContext));
        }
        return *new ExpressionListExprStmt(sourceSpan(), clonedExprs);
    }
};

/** Contains a series of expressions within a lexical scope, i.e. those contained within { and }.
 * TODO: inherit from ExpressionList? */
class CompoundExprStmt : public ExprStmt {
    scope::SymbolTable *scope_;
    gc_ref_vector<ExprStmt> expressions_;
public:
    CompoundExprStmt(
        source::SourceSpan sourceSpan,
        scope::StorageKind storageKind,
        const gc_ref_vector<ExprStmt> &expressions,
        const std::string &scopeName)
        : CompoundExprStmt(sourceSpan, expressions, new scope::ScopeSymbolTable(storageKind, scopeName))
    {

    }
    CompoundExprStmt(
        source::SourceSpan sourceSpan,
        const gc_ref_vector<ExprStmt> &expressions,
        scope::SymbolTable *symbolTable)
        : ExprStmt(sourceSpan),
          scope_{symbolTable},
          expressions_{expressions}
    {

    }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Compound; }
    
    scope::SymbolTable &scope() { return *scope_; }

    type::Type &exprType() const override {
        ASSERT(expressions_.size() > 0);
        return expressions_.back().get().exprType();
    };

    bool canWrite() const override { return false; };

    gc_ref_vector<ExprStmt> expressions() const {
        return expressions_;
    }

    void append(ExprStmt &exprStmt) {
        expressions_.emplace_back(exprStmt);
    }

    virtual void accept(AstVisitor &visitor) override {
        visitor.visitingCompoundExpr(*this);

        if(visitor.shouldVisitChildren()) {
            //Copy expressions_ because expressions can be added while visiting children
            //(adding items to vector invalidates iterator)
            gc_ref_vector<ExprStmt> exprs = expressions_;
            for (ExprStmt &stmt : exprs) {
                stmt.acceptVisitor(visitor);
            }
        }
        visitor.visitedCompoundExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ExprStmt> clonedExprs;
        clonedExprs.reserve(expressions_.size());
        for(ExprStmt &exprStmt : expressions_) {
            clonedExprs.emplace_back(exprStmt.deepCopyExpandTemplate(expansionContext));
        }
        return *new CompoundExprStmt(sourceSpan(), scope_->storageKind(), clonedExprs, scope_->name());
    }

};

/**
 * template ([template arg], ...) { <expression lists> }
 */
class AnonymousTemplateExprStmt : public VoidExprStmt {

protected:
    gc_ref_vector<ast::TemplateParameter> parameters_;
    ast::ExprStmt &body_;

    gc_ref_vector <TemplateParameter> deepCopyTemplateParameters() const {
        gc_ref_vector<TemplateParameter> copiedParameters;
        copiedParameters.reserve(parameters_.size());
        for(TemplateParameter &f : parameters_) {
            copiedParameters.emplace_back(f.deepCopyForTemplate());
        }
        return copiedParameters;
    }

public:
    AnonymousTemplateExprStmt(
        const source::SourceSpan &sourceSpan,
        const gc_ref_vector<ast::TemplateParameter> &parameters,
        ast::ExprStmt &body)
        : VoidExprStmt(sourceSpan),
          parameters_{parameters},
          body_{body} {}
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::AnonymousTemplate; }

    gc_ref_vector<ast::TemplateParameter> &parameters() { return parameters_; }

    ExprStmt &body() { return body_; }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<TemplateParameter> copiedParameters = deepCopyTemplateParameters();
        auto &copiedBody = body_.deepCopyExpandTemplate(expansionContext);
        return *new AnonymousTemplateExprStmt(sourceSpan(), copiedParameters, copiedBody);
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingAnonymousTemplateExprStmt(*this);

        if(visitor.shouldVisitChildren()) {
            for(TemplateParameter &p : parameters_) {
                p.acceptVisitor(visitor);
            }
        }

        //Note that we do *not* visit body_ here.
        visitor.visitedAnonymousTemplateExprStmt(*this);
    }
};

/**
 * template <name> ([template arg], ...) <expression list>
 */
class NamedTemplateExprStmt : public AnonymousTemplateExprStmt {
    const Identifier name_;
public:
    NamedTemplateExprStmt(
        const source::SourceSpan &sourceSpan,
        const Identifier &name,
        const gc_ref_vector<ast::TemplateParameter> &parameters,
          ast::ExpressionListExprStmt &body)
        : AnonymousTemplateExprStmt(sourceSpan, parameters, body),
          name_{name} {}
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::NamedTemplate; }


    const Identifier &name() { return name_; }
    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ast::TemplateParameter> copiedParameters = deepCopyTemplateParameters();
        auto &copiedBody = body_.deepCopyExpandTemplate(expansionContext);
        return *new NamedTemplateExprStmt(sourceSpan(), name_, copiedParameters,
                                          downcast<ExpressionListExprStmt>(copiedBody));
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingNamedTemplateExprStmt(*this);

        if(visitor.shouldVisitChildren()) {
            for(TemplateParameter &p : parameters_) {
                p.acceptVisitor(visitor);
            }
        }

        //Note that we do *not* visit body_ here.
        visitor.visitedNamedTemplateExprStmt(*this);
    }
};

class TemplateExpansionExprStmt : public ExprStmt {
    MultiPartIdentifier templateName_;
    AnonymousTemplateExprStmt *template_ = nullptr;
    //TODO:  convert below to vector of TemplateArgument*
    gc_ref_vector<ast::TypeRef> typeArguments_;
    ast::ExprStmt *expandedTemplate_ = nullptr;
    scope::ScopeSymbolTable templateParameterScope_;

public:
    TemplateExpansionExprStmt(
        source::SourceSpan sourceSpan,
        const MultiPartIdentifier &templateName,
        const gc_ref_vector<TypeRef> &typeArguments)
        : ExprStmt(sourceSpan),
          templateName_{templateName},
          typeArguments_{typeArguments},
          templateParameterScope_{scope::StorageKind::TemplateParameter, templateName.qualifedName() + scope::ScopeSeparator + "expanded_arguments"}
    { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::TemplateExpansion; }
    
    bool canWrite() const override { return false; }
    type::Type &exprType() const override {
        ASSERT(expandedTemplate_)
        return expandedTemplate_->exprType();
    };

    AnonymousTemplateExprStmt &templ() const { return *template_; }
    void setTempl(AnonymousTemplateExprStmt &templ) {
        ASSERT(&templ);
        template_ = &templ;
    }

    const MultiPartIdentifier &templateName() const { return templateName_; }
    gc_ref_vector<ast::TypeRef> typeArguments() const { return typeArguments_; }

    scope::SymbolTable &templateParameterScope() { return templateParameterScope_; }

    void setExpandedTemplate(ExprStmt *expandedTemplate) { expandedTemplate_ = expandedTemplate; }
    ast::ExprStmt *expandedTemplate() { return expandedTemplate_; }

    void accept(AstVisitor &visitor) override {
        if(visitor.shouldVisitChildren()) {
            for(TypeRef &arg : typeArguments_) {
                arg.acceptVisitor(visitor);
            }
            visitor.visitingTemplateExpansionExprStmt(*this);
            if(expandedTemplate_) {
                expandedTemplate_->acceptVisitor(visitor);
            }
        } else {
            visitor.visitingTemplateExpansionExprStmt(*this);
        }
        visitor.visitedTemplateExpansionExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new TemplateExpansionExprStmt(sourceSpan(), templateName_, deepCopyVector(typeArguments_));
    }
};


/** Represents a literal boolean */
class LiteralBoolExprStmt : public ExprStmt {
    bool const value_;
public:
    LiteralBoolExprStmt(source::SourceSpan sourceSpan, const bool value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::LiteralBool; }
    
    type::Type &exprType() const override { return type::ScalarType::Bool; }
    bool value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralBoolExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralBoolExprStmt(sourceSpan(), value_);
    }
};

/** Represents an expression that is a literal 32 bit integer. */
class LiteralInt32ExprStmt : public ExprStmt {
    int const value_;
public:
    LiteralInt32ExprStmt(source::SourceSpan sourceSpan, const int value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::LiteralInt32; }
    
    type::Type &exprType() const override { return type::ScalarType::Int32; }
    int value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralInt32Expr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralInt32ExprStmt(sourceSpan(), value_);
    }
};

/** Represents an expression that is a literal float. */
class LiteralFloatExprStmt : public ExprStmt {
    float const value_;
public:
    LiteralFloatExprStmt(source::SourceSpan sourceSpan, const float value) : ExprStmt(sourceSpan), value_(value) {}
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::LiteralFloat; }
    
    
    type::Type &exprType() const override {  return type::ScalarType::Float; }
    float value() const { return value_; }

    bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralFloatExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralFloatExprStmt(sourceSpan(), value_);
    }

};

enum class UnaryOperationKind : unsigned char {
    Not,
    PreIncrement,
    PreDecrement
};
std::string to_string(UnaryOperationKind type);

class UnaryExprStmt : public ExprStmt {
    const source::SourceSpan operatorSpan_;
    ExprStmt *valueExpr_;
    const UnaryOperationKind operation_;

public:
    /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
    UnaryExprStmt(source::SourceSpan sourceSpan, ExprStmt &valueExpr, UnaryOperationKind operation, source::SourceSpan operatorSpan)
        : ExprStmt{sourceSpan}, operatorSpan_{operatorSpan}, valueExpr_{&valueExpr}, operation_{operation} {

        ASSERT(&valueExpr_);
    }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Unary; }
    
    
    source::SourceSpan operatorSpan() { return operatorSpan_; }

    type::Type &exprType() const override { return type::ScalarType::Bool; }

    ExprStmt &valueExpr() const { return *valueExpr_; }
    void setLValue(ExprStmt *newLValue) {
        ASSERT(newLValue);
        valueExpr_ = newLValue;
    }

    virtual bool canWrite() const override { return false; };

    UnaryOperationKind operation() const { return operation_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingUnaryExpr(*this);

        if(visitor.shouldVisitChildren()) {
            valueExpr_->acceptVisitor(visitor);
        }
        visitor.visitedUnaryExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new UnaryExprStmt(sourceSpan(), valueExpr_->deepCopyExpandTemplate(expansionContext), operation_, operatorSpan_);
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

/** Represents a binary expression, i.e. 1 + 2 or foo + bar */
class BinaryExprStmt : public ExprStmt {
    ExprStmt *lValue_;
    ExprStmt *rValue_;
    const BinaryOperationKind operation_;
    const source::SourceSpan operatorSpan_;

public:
    /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
    BinaryExprStmt(source::SourceSpan sourceSpan,
    ExprStmt &lValue,
    BinaryOperationKind operation,
    source::SourceSpan operatorSpan,
    ExprStmt& rValue)
    : ExprStmt{sourceSpan},
      lValue_{&lValue},
      rValue_{&rValue},
      operation_{operation},
      operatorSpan_{operatorSpan}
    {

        ASSERT(&lValue_);
        ASSERT(&rValue_);
    }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Binary; }
    
    source::SourceSpan operatorSpan() { return operatorSpan_; }

    /** This is the type of the result, which may be different than the type of the operands depending on the operation type,
     * because some operation types (e.g. equality, logical and, or, etc) always yield boolean values.  */
    type::Type &exprType() const override {
        if(isComparison()) {
            return type::ScalarType::Bool;
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
    type::Type &operandsType() const {
        ASSERT(rValue_->exprType().isSameType(lValue_->exprType().actualType()));
        return rValue_->exprType();
    }

    ExprStmt &lValue() const { return *lValue_; }
    void setLValue(ExprStmt &newLValue) { lValue_ = &newLValue; }

    ExprStmt &rValue() const { return *rValue_; }
    void setRValue(ExprStmt &newRValue) { rValue_ = &newRValue; }

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

    void accept(AstVisitor &visitor) override {
        visitor.beforeVisit(*this);

        if(visitor.shouldVisitChildren()) {
            lValue_->acceptVisitor(visitor);
            rValue_->acceptVisitor(visitor);
        }
        visitor.visitedBinaryExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new BinaryExprStmt(sourceSpan(),
                              lValue_->deepCopyExpandTemplate(expansionContext),
                              operation_,
                              operatorSpan_,
                              rValue_->deepCopyExpandTemplate(expansionContext));
    }
};

enum class VariableAccess : unsigned char {
    Read,
    Write
};

/**
 * Represents a reference to a previously declared variable.
 */
class VariableRefExprStmt : public ExprStmt {
    scope::Symbol *symbol_ = nullptr;
    VariableAccess access_ = VariableAccess::Read;
protected:
    const MultiPartIdentifier name_;

    explicit VariableRefExprStmt(const VariableRefExprStmt &from) : ExprStmt(from), access_{from.access_}, name_{from.name_} {

    };

public:
    VariableRefExprStmt(source::SourceSpan sourceSpan, const MultiPartIdentifier &name, VariableAccess access = VariableAccess::Read)
        : ExprStmt(sourceSpan), access_{access}, name_{ name } { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::VariableRef; }

    type::Type &exprType() const override {
        return symbol_ ? symbol_->type() : type::UnresolvedType::Instance;
    }

    const MultiPartIdentifier &name() const { return name_; }
    std::string toString() const { return name_.qualifedName() + ":" + this->exprType().nameForDisplay(); }

    scope::Symbol *symbol() {
        return symbol_;
    }
    void setSymbol(scope::Symbol &symbol) {
        symbol_ = &symbol;
    }

    bool canWrite() const override { return true; };

    VariableAccess variableAccess() const { return access_; }
    void setVariableAccess(VariableAccess access) { access_ = access; }

    virtual void accept(AstVisitor &visitor) override {
        visitor.visitVariableRefExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new VariableRefExprStmt(*this);
    }
};


/** Defines a variable and references it. */
class VariableDeclExprStmt : public VariableRefExprStmt {
    TypeRef& typeRef_;

private:
    explicit VariableDeclExprStmt(const VariableDeclExprStmt &copyFrom) : VariableRefExprStmt(copyFrom), typeRef_{copyFrom.typeRef_.deepCopyForTemplate()} { }
public:
    VariableDeclExprStmt(source::SourceSpan sourceSpan, const MultiPartIdentifier &name, TypeRef& typeRef, VariableAccess access = VariableAccess::Read)
        : VariableRefExprStmt(sourceSpan, name, access),
          typeRef_(typeRef)
    {
    }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::VariableDecl; }

    TypeRef &typeRef() { return typeRef_; }
    virtual type::Type &exprType() const override { return typeRef_.type(); }

    virtual bool canWrite() const override { return true; };

    void accept(AstVisitor &visitor) override {
        visitor.beforeVisit(*this);

        if(visitor.shouldVisitChildren()) {
            typeRef_.acceptVisitor(visitor);
        }

        visitor.visitedVariableDeclExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        VariableDeclExprStmt &varDecl = *new VariableDeclExprStmt(*this);
        return varDecl;
    }


};

enum class CastKind : unsigned char {
    Explicit,
    Implicit
};

/** Represents a cast expression... i.e. foo:int= cast<int>(someDouble); */
class CastExprStmt : public ExprStmt {
    TypeRef &toTypeRef_;
    ExprStmt &valueExpr_;
    const CastKind castKind_;

public:
    /** Use this constructor when the type::Type of the cast *is* known in advance. */
    CastExprStmt(source::SourceSpan sourceSpan, type::Type &toType, ExprStmt& valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toTypeRef_(*new KnownTypeRef(sourceSpan, toType)), valueExpr_(valueExpr), castKind_(castKind)
    {
        ASSERT(&toType);
        ASSERT(&valueExpr);
    }

    /** Use this constructor when the type::Type of the cast is *not* known in advance. */
    CastExprStmt(source::SourceSpan sourceSpan, TypeRef &toType, ExprStmt &valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toTypeRef_(toType), valueExpr_(valueExpr), castKind_(castKind) { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Cast; }

    static inline CastExprStmt &createImplicit(ExprStmt &valueExpr, type::Type &toType) {
        return *new CastExprStmt(valueExpr.sourceSpan(), *new KnownTypeRef(valueExpr.sourceSpan(), toType), valueExpr, CastKind::Implicit);
    }

    TypeRef &toTypeRef() { return toTypeRef_; }
    type::Type &exprType() const  override{ return toTypeRef_.type(); }
    CastKind castKind() const { return castKind_; }

    virtual bool canWrite() const override { return false; };

    ExprStmt &valueExpr() const { return valueExpr_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingCastExprStmt(*this);

        if(visitor.shouldVisitChildren()) {
            toTypeRef_.acceptVisitor(visitor);
            valueExpr_.acceptVisitor(visitor);
        }
    
        visitor.visitedCastExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new CastExprStmt(sourceSpan(), toTypeRef_.deepCopyForTemplate(), valueExpr_.deepCopyExpandTemplate(expansionContext), castKind_);
    }
};

/** Represents a new expression... i.e. foo:int= new<int>(someDouble); */
class NewExprStmt : public ExprStmt {
    TypeRef &typeRef_;
public:

    NewExprStmt(source::SourceSpan sourceSpan, TypeRef &typeRef)
        : ExprStmt(sourceSpan), typeRef_(typeRef)
    {
        ASSERT(&typeRef);
    }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::New; }

    type::Type &exprType() const  override { return typeRef_.type(); }
    TypeRef &typeRef() const { return typeRef_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitingNewExpr(*this);

        if(visitor.shouldVisitChildren()) {
            typeRef_.acceptVisitor(visitor);
        }

        visitor.visitedNewExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new NewExprStmt(sourceSpan(), typeRef_.deepCopyForTemplate());
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
               ExprStmt& condition,
               ExprStmt& trueExpr,
               ExprStmt* elseExpr)
        : ExprStmt(sourceSpan),
          condition_{ &condition },
          thenExpr_{ &trueExpr },
          elseExpr_{ elseExpr } {
        ASSERT(&condition_);
        ASSERT(&thenExpr_);
    }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::If; }

    type::Type &exprType() const override {
        if(elseExpr_ == nullptr || !thenExpr_->exprType().isSameType(elseExpr_->exprType())) {
            return type::ScalarType::Void;
        }

        return thenExpr_->exprType();
    }

    ExprStmt &condition() const { return *condition_; }
    void setCondition(ExprStmt &newCondition){ condition_ = &newCondition; }

    ExprStmt &thenExpr() const { return *thenExpr_; }
    void setThenExpr(ExprStmt &newThenExpr) { thenExpr_ = &newThenExpr; }

    ExprStmt *elseExpr() const { return elseExpr_; }
    void setElseExpr(ExprStmt &newElseExpr) { elseExpr_ = &newElseExpr; }

    bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.beforeVisit(*this);

        if(visitor.shouldVisitChildren()) {
            condition_->acceptVisitor(visitor);
            thenExpr_->acceptVisitor(visitor);
            if(elseExpr_)
                elseExpr_->acceptVisitor(visitor);
        }

        visitor.visitedIfExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new IfExprStmt(
            sourceSpan(),
            condition_->deepCopyExpandTemplate(expansionContext),
            thenExpr_->deepCopyExpandTemplate(expansionContext),
            elseExpr_ ? &elseExpr_->deepCopyExpandTemplate(expansionContext) : nullptr);
    }
};

class WhileExprStmt : public VoidExprStmt {
    ExprStmt* condition_;
    ExprStmt& body_;
public:

    /** Note:  assumes ownership of condition, truePart and falsePart.  */
    WhileExprStmt(source::SourceSpan sourceSpan,
              ExprStmt& condition,
              ExprStmt& body)
        : VoidExprStmt(sourceSpan),
          condition_{ &condition },
          body_{ body } {
        ASSERT(&condition_);
        ASSERT(&body_)
    }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::While; }
    
    
    ExprStmt &condition() const { return *condition_; }

    void setCondition(ExprStmt &newCondition) {
        ASSERT(&newCondition);
        condition_ = &newCondition;
    }

    ExprStmt &body() const { return body_; }

    void accept(AstVisitor &visitor) override {
        visitor.beforeVisit(*this);

        if(visitor.shouldVisitChildren()) {
            condition_->acceptVisitor(visitor);
            body_.acceptVisitor(visitor);
        }

        visitor.visitedWhileExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new WhileExprStmt(
            sourceSpan(),
            condition_->deepCopyExpandTemplate(expansionContext),
            body_.deepCopyExpandTemplate(expansionContext));
    }
};

class ParameterDef : public AstNode {
    const Identifier name_;
    TypeRef& typeRef_;
    scope::VariableSymbol *symbol_ = nullptr;
public:
    ParameterDef(source::SourceSpan span, const Identifier &name, TypeRef &typeRef)
        : AstNode(span), name_{name}, typeRef_{typeRef} { }
    AstNodeKind astNodeKind() override { return AstNodeKind::ParameterDef; }

    const Identifier &name() { return name_; }
    type::Type &type() { return typeRef_.type(); }
    TypeRef &typeRef() { return typeRef_; }

    scope::VariableSymbol *symbol() {
        ASSERT(symbol_);
        return symbol_;
    }
    void setSymbol(scope::VariableSymbol &symbol) { symbol_ = &symbol; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingParameterDef(*this);
        if(visitor.shouldVisitChildren()) {
            typeRef_.acceptVisitor(visitor);
        }
        visitor.visitedParameterDef(*this);
    }

    ParameterDef &deepCopy() {
        //I'm not sure that the typeRef_ really needs to be copied.
        return *new ParameterDef(sourceSpan(), name_, typeRef_.deepCopyForTemplate());
    }
};
type::FunctionType &createFunctionType(type::Type &returnType, const gc_ref_vector<ParameterDef> &parameters);

class FuncDefExprStmt : public VoidExprStmt {
    const Identifier name_;
    scope::FunctionSymbol *symbol_= nullptr;
    scope::ScopeSymbolTable parameterScope_;
    TypeRef &returnTypeRef_;
    gc_ref_vector<ParameterDef> parameters_;
    ExprStmt* body_;
    type::FunctionType &functionType_;

public:
    FuncDefExprStmt(
        source::SourceSpan sourceSpan,
        const Identifier &name,
        TypeRef& returnTypeRef,
        gc_ref_vector<ParameterDef> parameters,
        ExprStmt& body
    ) : VoidExprStmt(sourceSpan),
        name_{name},
        parameterScope_{scope::StorageKind::Argument, name.text() + scope::ScopeSeparator + "parameters"},
        returnTypeRef_{returnTypeRef},
        parameters_{parameters},
        body_{&body},
        functionType_{createFunctionType(returnTypeRef.type(), parameters)}
    {
        parameterScope_.name() = name_.text() + "-parameters";
    }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::FuncDef; }

    const Identifier &name() const { return name_; }
    type::Type &returnType() const { return *functionType_.returnType(); }
    TypeRef &returnTypeRef() const { return returnTypeRef_; }
    type::FunctionType &functionType() { return functionType_; }
    scope::SymbolTable &parameterScope() { return parameterScope_; };
    ExprStmt &body() const { return *body_; }
    void setBody(ExprStmt &body) {
        ASSERT(&body);
        body_ = &body;
    }

    scope::FunctionSymbol *symbol() const {
        ASSERT(symbol_ && "Function symbol not resolved yet")
        return symbol_;
    }

    void setSymbol(scope::FunctionSymbol &symbol) { symbol_ = &symbol; }

    gc_ref_vector<ParameterDef> parameters() {
       return parameters_;
    }

    virtual void accept(AstVisitor &visitor) override {
        visitor.visitingFuncDefExprStmt(*this);
        if(visitor.shouldVisitChildren()) {
            returnTypeRef_.acceptVisitor(visitor);
            for(const auto p : parameters_) {
                p.get().acceptVisitor(visitor);
            }
            body_->acceptVisitor(visitor);
        }
        visitor.visitedFuncDeclStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ParameterDef> clonedParameters;
        for(ParameterDef &p : parameters_) {
            clonedParameters.emplace_back(p.deepCopy());
        }
        return *new FuncDefExprStmt(sourceSpan(),
                               name_,
                               returnTypeRef_.deepCopyForTemplate(),
                               clonedParameters,
                                body_->deepCopyExpandTemplate(expansionContext));
    }
};

class MethodRefExprStmt : public ExprStmt {
    const Identifier name_;
    scope::FunctionSymbol *symbol_ = nullptr;
public:
    explicit MethodRefExprStmt(const Identifier &name) : ExprStmt(name.span()), name_{ name } {
        ASSERT(name.text().size() > 0);
    }

    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::MethodRef; }

    virtual type::Type &exprType() const override {
        ASSERT(symbol_);
        return symbol_->type();
    }

    virtual const Identifier &name() const { return name_; }
    std::string toString() const { return name_.text() + ":" + this->exprType().nameForDisplay(); }

    scope::FunctionSymbol *symbol() { return symbol_; }
    void setSymbol(scope::FunctionSymbol &symbol) {
        symbol_ = &symbol;
    }

    bool canWrite() const override { return true; };

    void accept(AstVisitor &visitor) override {
        visitor.visitMethodRefExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new MethodRefExprStmt(name_);
    }
};

class FuncCallExprStmt : public ExprStmt {
    ExprStmt *instanceExpr_;
    source::SourceSpan openParenSpan_;
    ExprStmt &funcExpr_;
    gc_ref_vector<ExprStmt> arguments_;
public:
    FuncCallExprStmt(
        const source::SourceSpan &span,
        ExprStmt *instanceExpr,
        const source::SourceSpan &openParenSpan,
        ast::ExprStmt &funcExpr,
        const gc_ref_vector<ExprStmt> &arguments
    ) : ExprStmt(span),
        instanceExpr_{instanceExpr},
        openParenSpan_{openParenSpan},
        funcExpr_{funcExpr},
        arguments_{arguments} { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::FuncCall; }
    source::SourceSpan openParenSpan() { return openParenSpan_; }

    ExprStmt &funcExpr() const { return funcExpr_; }
    ExprStmt *instanceExpr() const { return instanceExpr_; }

    bool canWrite() const override { return false; };

    type::Type &exprType() const override {
        auto &functionType = downcast<type::FunctionType>(funcExpr_.exprType());
        return *functionType.returnType();
    }

    const gc_ref_vector<ExprStmt> &arguments() const {
        return arguments_;
    }

    void replaceArgument(size_t index, ExprStmt &newExpr) {
        ASSERT(index < arguments_.size());
        arguments_[index] = newExpr;
    }

    size_t argCount() { return arguments_.size(); }

    void accept(AstVisitor &visitor) override {
        visitor.vistingFuncCallExprStmt(*this);
        if(visitor.shouldVisitChildren()) {
            funcExpr_.acceptVisitor(visitor);
            if(instanceExpr_) {
                instanceExpr_->acceptVisitor(visitor);
            }
            for (auto argument : arguments_) {
                argument.get().acceptVisitor(visitor);
            }
        }
        visitor.visitedFuncCallExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ExprStmt> clonedArguments;
        clonedArguments.reserve(arguments_.size());
        for(ExprStmt &a : arguments_) {
            clonedArguments.emplace_back(a.deepCopyExpandTemplate(expansionContext));
        }
        return *new FuncCallExprStmt(sourceSpan(),
                                 (instanceExpr_ ? &instanceExpr_->deepCopyExpandTemplate(expansionContext) : nullptr),
                                openParenSpan_,
                                funcExpr_.deepCopyExpandTemplate(expansionContext),
                                clonedArguments);
    }
};

class NamespaceExprStmt : public VoidExprStmt {
    MultiPartIdentifier qualifiedName_;
    ExpressionListExprStmt &body_;
    scope::SymbolTable *scope_;
public:
    NO_COPY_NO_ASSIGN(NamespaceExprStmt)

    NamespaceExprStmt(const source::SourceSpan &sourceSpan, const MultiPartIdentifier &qualifiedName, ExpressionListExprStmt &body)
        : VoidExprStmt(sourceSpan),
          qualifiedName_{qualifiedName}, body_{body} { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Namespace; }

    scope::SymbolTable &scope() {
        ASSERT(scope_);
        return *scope_;
    }

    void setScope(scope::SymbolTable &scope) {
        scope_ = &scope;
    }

    const MultiPartIdentifier &qualifiedName() { return qualifiedName_; }
    ExpressionListExprStmt &body() { return body_; }


    void accept(AstVisitor &visitor) override {
        visitor.visitingNamespaceExpr(*this);
        body_.acceptVisitor(visitor);
        visitor.visitedNamespaceExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new NamespaceExprStmt(sourceSpan(), qualifiedName_,
                                      downcast<ExpressionListExprStmt>(body_.deepCopyExpandTemplate(expansionContext)));
    }
};

class ClassDefinition : public VoidExprStmt {
    Identifier name_;
    CompoundExprStmt &body_;
protected:
    ClassDefinition(
        source::SourceSpan span,
        const Identifier &name,
        ast::CompoundExprStmt &body
    ) : VoidExprStmt{span},
        name_{name},
        body_{body}
    { }
public:
    /** The type of the class being defined. */
    virtual type::Type &definedType() const = 0;

    CompoundExprStmt &body() const { return body_; }
    Identifier name() const { return name_; }
};

inline gc_ref_vector<type::Type> toVectorOfType(const gc_ref_vector<TemplateArgument> &arguments) {
    gc_ref_vector<type::Type> vectorOfTypes;
    vectorOfTypes.reserve(arguments.size());
    for(TemplateArgument &arg : arguments) {
        vectorOfTypes.emplace_back(arg.typeRef().type());
    }
    return vectorOfTypes;
}

class CompleteClassDefExprStmt : public ClassDefinition {
    gc_ref_vector<TemplateArgument> templateArguments_;
    type::Type &definedType_;

public:
    CompleteClassDefExprStmt(
        source::SourceSpan span,
        const Identifier &name,
        ast::CompoundExprStmt &body
    ) : ClassDefinition{span, name, body},
        definedType_{*new type::ClassType(AstNode::nodeId(), name.text(), toVectorOfType(templateArguments_))}
    { }

    CompleteClassDefExprStmt(
        source::SourceSpan span,
        const Identifier &name,
        const gc_ref_vector<TemplateArgument> &templateArgs,
        ast::CompoundExprStmt &body
    ) : ClassDefinition{span, name, body},
        templateArguments_{templateArgs},
        definedType_{*new type::ClassType(AstNode::nodeId(), name.text(), toVectorOfType(templateArguments_))}
    { }
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::CompleteClassDef; }

    /** The type of the class being defined. */
    type::Type &definedType() const override { return definedType_; }

    bool hasTemplateArguments() { return !templateArguments_.empty(); }
    gc_ref_vector<TemplateArgument> templateArguments() { return templateArguments_; }
    
    void populateClassType() {
        auto &ct = downcast<type::ClassType>(definedType_);

        for (auto &&variable : this->body().scope().variables()) {
            ct.addField(variable.get().name(), variable.get().type());
        }

        for (auto &&method : this->body().scope().functions()) {
            method.get().setThisSymbol(new scope::VariableSymbol("this", this->definedType()));
            ct.addMethod(method.get().name(), method);
        }
    }

    void accept(AstVisitor &visitor) override {
        visitor.beforeVisit(*this);
        if (visitor.shouldVisitChildren()) {
            body().acceptVisitor(visitor);
        }
        visitor.visitedCompleteClassDefExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        auto &completeClassDef = *new CompleteClassDefExprStmt(
            sourceSpan(),
            name(),
            templateArguments_, downcast<CompoundExprStmt>(body().deepCopyExpandTemplate(expansionContext)));

        return completeClassDef;
    }
};

class GenericClassDefExprStmt : public ClassDefinition {
    gc_ref_vector<ast::TemplateParameter> templateParameters_;
    type::GenericType *definedType_;
    scope::TypeSymbol *symbol_ = nullptr;

    inline static std::vector<std::string> getTemplateParameterNames(const gc_ref_vector<TemplateParameter> &parameters) {
        std::vector<std::string> names;
        for(TemplateParameter& parameter : parameters) {
            names.push_back(parameter.name().text());
        }
        return names;
    }
private:
    explicit GenericClassDefExprStmt(const GenericClassDefExprStmt &copyFrom)
        : ClassDefinition(copyFrom),
          templateParameters_{deepCopyVector(copyFrom.templateParameters_)},
          definedType_{new type::GenericType(nodeId(), name().text(), getTemplateParameterNames(templateParameters_))}
    {

    }

public:
    GenericClassDefExprStmt(
        source::SourceSpan span,
        const Identifier &name,
        const gc_ref_vector<TemplateParameter> &templateParameters,
        ast::CompoundExprStmt &body
    ) : ClassDefinition{span, name, body},
        templateParameters_{templateParameters},
        definedType_{new type::GenericType(nodeId(), name.text(), getTemplateParameterNames(templateParameters))}
    { }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::GenericClassDef; }
    
    /** The type of the class being defined. */
    type::Type &definedType() const override { return *definedType_; }

    scope::TypeSymbol *symbol() { ASSERT(symbol_); return symbol_; }
    void setSymbol(scope::TypeSymbol &s) { symbol_ = &s; }

    const gc_ref_vector<TemplateParameter> &templateParameters() {
        return templateParameters_;
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingGenericClassDefExprStmt(*this);

        if(visitor.shouldVisitChildren()){
            body().acceptVisitor(visitor);
        }

        visitor.visitedGenericClassDefExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        switch(expansionContext.expansionKind) {
            case ExpansionKind::AnonymousTemplate: {
                auto &completeClassDef = *new CompleteClassDefExprStmt(
                    sourceSpan(),
                    name(),
                    expansionContext.args, downcast<CompoundExprStmt>(body().deepCopyExpandTemplate(expansionContext)));
    
                downcast<type::ClassType>(completeClassDef.definedType()).setGenericType(definedType_);
                return completeClassDef;
            }
            case ExpansionKind::NamedTemplate:
                return *new GenericClassDefExprStmt(*this);
            default:
                ASSERT_FAIL("Unhandled ExpansionKind")
        }
        //Only once inside an expanded template, a generic class is not generic anymore so construct CompleteClassDefExprStmt
        auto &completeClassDef = *new CompleteClassDefExprStmt(
            sourceSpan(),
            name(),
            expansionContext.args, downcast<CompoundExprStmt>(body().deepCopyExpandTemplate(expansionContext)));
    
        downcast<type::ClassType>(completeClassDef.definedType()).setGenericType(definedType_);

        return completeClassDef;
    }
};

class DotExprStmt : public ExprStmt {
    source::SourceSpan dotSourceSpan_;
    ExprStmt &lValue_;
    const Identifier memberName_;
    type::ClassField *field_ = nullptr;
    bool isWrite_ = false;
public:
    DotExprStmt(const source::SourceSpan &sourceSpan,
            const source::SourceSpan &dotSourceSpan,
            ExprStmt &lValue,
            const Identifier &memberName)
        : ExprStmt(sourceSpan), dotSourceSpan_{dotSourceSpan}, lValue_{lValue}, memberName_{memberName} { }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Dot; }

    source::SourceSpan dotSourceSpan() { return dotSourceSpan_; };

    bool canWrite() const override { return true; };

    bool isWrite() { return isWrite_; }
    void setIsWrite(bool isWrite) {
        //Eventually, this will need to propagate to the deepest DotExprStmt, not the first one that's invoked...
        isWrite_ = isWrite;
    }

    ExprStmt &lValue() { return lValue_; }
    const Identifier &memberName() { return memberName_; }

    type::ClassField *field() { return field_; }
    void setField(type::ClassField *field) { field_ = field; }

    type::Type &exprType() const override {
        ASSERT(field_ && "Field must be resolved first");
        return field_->type();
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingDotExpr(*this);
        if (visitor.shouldVisitChildren()) {
            lValue_.acceptVisitor(visitor);
        }
        visitor.visitedDotExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new DotExprStmt(sourceSpan(), dotSourceSpan_, lValue_.deepCopyExpandTemplate(expansionContext), memberName_);
    }
};

class AssertExprStmt : public VoidExprStmt {
    ast::ExprStmt *condition_;
public:
    AssertExprStmt(const source::SourceSpan &sourceSpan, ExprStmt &condition)
        : VoidExprStmt(sourceSpan), condition_{&condition} {
        ASSERT(condition_);
    }
    
    virtual ExprStmtKind exprStmtKind() override { return ExprStmtKind::Assert; }
    
    ast::ExprStmt &condition() {
        return *condition_;
    }
    void setCondition(ast::ExprStmt &condition) { condition_ = &condition; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingAssertExprStmt(*this);
        if(visitor.shouldVisitChildren()) {
            condition_->acceptVisitor(visitor);
        }
        visitor.visitedAssertExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new AssertExprStmt(sourceSpan(), condition_->deepCopyExpandTemplate(expansionContext));
    }
};

class Module : public AstNode {
    std::string name_;
    CompoundExprStmt &body_;
    //gc_unordered_map<std::string, TemplateExprStmt*> templates_;
public:
    Module(const std::string &name, CompoundExprStmt& body)
        : AstNode(body.sourceSpan()), name_{name}, body_{body} {
    }
    AstNodeKind astNodeKind() override { return AstNodeKind::Module; }
    
    std::string name() const { return name_; }

    scope::SymbolTable &scope() { return body_.scope(); }

    CompoundExprStmt &body() { return body_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingModule(*this);
        if(visitor.shouldVisitChildren()) {
            body_.acceptVisitor(visitor);
        }
        visitor.visitedModule(*this);
    }
};


/**
 * A kind of compilation context for templates, mainly...
 * FIXME: this class really needs a better name.
 */
class AnodeWorld : public gc {
    scope::ScopeSymbolTable globalScope_{scope::StorageKind::Global, "::"};
    gc_unordered_map<UniqueId, GenericClassDefExprStmt*> genericClassIndex_;
    gc_unordered_map<UniqueId, AnonymousTemplateExprStmt*> templateIndex_;
    gc_unordered_set<ast::AnonymousTemplateExprStmt*> expandingTemplates_;
public:
    NO_COPY_NO_ASSIGN(AnodeWorld)
    AnodeWorld() { }
    scope::SymbolTable &globalScope() { return globalScope_; }

    void addTemplate(AnonymousTemplateExprStmt &templateExprStmt) {
        templateIndex_.emplace(templateExprStmt.nodeId(), &templateExprStmt);
    }

    AnonymousTemplateExprStmt &getTemplate(UniqueId nodeId) {
        AnonymousTemplateExprStmt *templ = templateIndex_[nodeId];
        ASSERT(templ);
        return *templ;
    }

    void addGenericClassDefinition(GenericClassDefExprStmt &genericClass) {
        ASSERT(genericClassIndex_.find(genericClass.nodeId()) == genericClassIndex_.end())
        genericClassIndex_.emplace(genericClass.nodeId(), &genericClass);
    }

    GenericClassDefExprStmt &getGenericClassDefinition(UniqueId nodeId) {
        auto templ = genericClassIndex_[nodeId];
        ASSERT(templ);
        return *templ;
    }

    void addExpandingTemplate(ast::AnonymousTemplateExprStmt &templ) {
        ASSERT(expandingTemplates_.find(&templ) == expandingTemplates_.end())
        expandingTemplates_.insert(&templ);
    }

    bool isExpanding(ast::AnonymousTemplateExprStmt &templ) {
        return expandingTemplates_.count(&templ) > 0;
    }

    void removeExpandingTemplate(ast::AnonymousTemplateExprStmt &templ) {
        expandingTemplates_.erase(&templ);
    }
};


}}}
