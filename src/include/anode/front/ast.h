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
public:
    //NO_COPY_NO_ASSIGN(AstNode)
    AstNode();

    virtual ~AstNode() {
        astNodesDestroyedCount++;
    }

    UniqueId nodeId() { return nodeId_; }

    virtual void accept(AstVisitor &visitor) = 0;
};

/** A statement any kind of non-terminal that may appear within a CompoundStatement (i.e. between { and }), or
 * at the global scope, i.e. global variables, assignments, class definitions,
 */
class Stmt : public AstNode {
protected:
    const source::SourceSpan sourceSpan_;
    explicit Stmt(const source::SourceSpan &sourceSpan) : sourceSpan_(sourceSpan) { }
    explicit Stmt(const Stmt &from) : AstNode(from), sourceSpan_{from.sourceSpan_} {}
public:
    //virtual StmtKind stmtKind() const = 0;

    const source::SourceSpan &sourceSpan() const {
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

    virtual type::Type &type() const = 0;
    virtual const MultiPartIdentifier &name() const = 0;
    virtual TypeRef &deepCopyForTemplate() const = 0;
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
            ta.accept(visitor);
        }
        visitor.visitedResolutionDeferredTypeRef(*this);
    }

    TypeRef& deepCopyForTemplate() const override {
        gc_ref_vector<ResolutionDeferredTypeRef> templateArgs;
        for(ResolutionDeferredTypeRef &ta : templateArgs_) {
            templateArgs.emplace_back(upcast<ResolutionDeferredTypeRef>(ta.deepCopyForTemplate()));
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
class TemplateParameter : public Stmt {
    const Identifier name_;
public:
    TemplateParameter(const source::SourceSpan &sourceSpan, const Identifier &name) : Stmt(sourceSpan), name_{name} { }

    const Identifier &name() const { return name_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitTemplateParameter(*this);
    }

    TemplateParameter &deepCopyForTemplate() const {
        return *new TemplateParameter(sourceSpan_, name());
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
class ExprStmt : public Stmt {
protected:
    explicit ExprStmt(const source::SourceSpan &sourceSpan) : Stmt(sourceSpan) { }
    explicit ExprStmt(const ExprStmt &from) : Stmt(from) { }
public:
    ~ExprStmt() override = default;

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
class ExpressionList : public ExprStmt {
    gc_ref_vector<ExprStmt> expressions_;
public:
    ExpressionList(source::SourceSpan sourceSpan, const gc_ref_vector<ExprStmt> &expressions)
        : ExprStmt(sourceSpan),
          expressions_{expressions}
    {
    }

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
            for (auto stmt : exprs) {
                stmt.get().accept(visitor);
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
        return *new ExpressionList(sourceSpan_, clonedExprs);
    }
};

/** Contains a series of expressions within a lexical scope, i.e. those contained within { and }.
 * TODO: inherit from ExpressionList? */
class CompoundExpr : public ExprStmt {
    scope::SymbolTable scope_;
    gc_ref_vector<ExprStmt> expressions_;
public:
    CompoundExpr(
        source::SourceSpan sourceSpan,
        scope::StorageKind storageKind,
        const gc_ref_vector<ExprStmt> &expressions,
        const std::string &scopeName)
        : ExprStmt(sourceSpan),
          scope_{storageKind, scopeName},
          expressions_{expressions}
    {

    }

    scope::SymbolTable &scope() { return scope_; }

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
                stmt.accept(visitor);
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
        return *new CompoundExpr(sourceSpan_, scope_.storageKind(), clonedExprs, scope_.name());
    }

};

/**
 * template ([template arg], ...) { <expression lists> }
 */
class AnonymousTemplateExprStmt : public VoidExprStmt {

protected:
    gc_ref_vector<ast::TemplateParameter> parameters_;
    ast::ExpressionList &body_;

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
        ast::ExpressionList &body)
        : VoidExprStmt(sourceSpan),
          parameters_{parameters},
          body_{body} {}

    gc_ref_vector<ast::TemplateParameter> &parameters() { return parameters_; }

    ExpressionList &body() { return body_; }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<TemplateParameter> copiedParameters = deepCopyTemplateParameters();
        auto &copiedBody = body_.deepCopyExpandTemplate(expansionContext);
        return *new AnonymousTemplateExprStmt(sourceSpan_, copiedParameters,
                                     upcast<ExpressionList>(copiedBody));
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingAnonymousTemplateExprStmt(*this);

        for(TemplateParameter &p : parameters_) {
            p.accept(visitor);
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
          ast::ExpressionList &body)
        : AnonymousTemplateExprStmt(sourceSpan, parameters, body),
          name_{name} {}


    const Identifier &name() { return name_; }
    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ast::TemplateParameter> copiedParameters = deepCopyTemplateParameters();
        auto &copiedBody = body_.deepCopyExpandTemplate(expansionContext);
        return *new NamedTemplateExprStmt(sourceSpan_, name_, copiedParameters,
                                     upcast<ExpressionList>(copiedBody));
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingNamedTemplateExprStmt(*this);

        for(TemplateParameter &p : parameters_) {
            p.accept(visitor);
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
    scope::SymbolTable templateParameterScope_;

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
                arg.accept(visitor);
            }
            visitor.visitingTemplateExpansionExprStmt(*this);
            if(expandedTemplate_) {
                expandedTemplate_->accept(visitor);
            }
        } else {
            visitor.visitingTemplateExpansionExprStmt(*this);
        }
        visitor.visitedTemplateExpansionExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new TemplateExpansionExprStmt(sourceSpan_, templateName_, deepCopyVector(typeArguments_));
    }
};


/** Represents a literal boolean */
class LiteralBoolExpr : public ExprStmt {
    bool const value_;
public:
    LiteralBoolExpr(source::SourceSpan sourceSpan, const bool value) : ExprStmt(sourceSpan), value_(value) {}
    type::Type &exprType() const override { return type::ScalarType::Bool; }
    bool value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralBoolExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralBoolExpr(sourceSpan_, value_);
    }
};

/** Represents an expression that is a literal 32 bit integer. */
class LiteralInt32Expr : public ExprStmt {
    int const value_;
public:
    LiteralInt32Expr(source::SourceSpan sourceSpan, const int value) : ExprStmt(sourceSpan), value_(value) {}
    type::Type &exprType() const override { return type::ScalarType::Int32; }
    int value() const { return value_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralInt32Expr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralInt32Expr(sourceSpan_, value_);
    }
};

/** Represents an expression that is a literal float. */
class LiteralFloatExpr : public ExprStmt {
    float const value_;
public:
    LiteralFloatExpr(source::SourceSpan sourceSpan, const float value) : ExprStmt(sourceSpan), value_(value) {}

    type::Type &exprType() const override {  return type::ScalarType::Float; }
    float value() const { return value_; }

    bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitLiteralFloatExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new LiteralFloatExpr(sourceSpan_, value_);
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
    UnaryExpr(source::SourceSpan sourceSpan, ExprStmt &valueExpr, UnaryOperationKind operation, source::SourceSpan operatorSpan)
        : ExprStmt{sourceSpan}, operatorSpan_{operatorSpan}, valueExpr_{&valueExpr}, operation_{operation} {

        ASSERT(&valueExpr_);
    }

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
            valueExpr_->accept(visitor);
        }
        visitor.visitedUnaryExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new UnaryExpr(sourceSpan_, valueExpr_->deepCopyExpandTemplate(expansionContext), operation_, operatorSpan_);
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
class BinaryExpr : public ExprStmt {
    ExprStmt *lValue_;
    ExprStmt *rValue_;
    const BinaryOperationKind operation_;
    const source::SourceSpan operatorSpan_;

public:
    /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
    BinaryExpr(source::SourceSpan sourceSpan,
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
        visitor.visitingBinaryExpr(*this);

        if(visitor.shouldVisitChildren()) {
            lValue_->accept(visitor);
            rValue_->accept(visitor);
        }
        visitor.visitedBinaryExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new BinaryExpr(sourceSpan_,
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
class VariableRefExpr : public ExprStmt {
    scope::Symbol *symbol_ = nullptr;
    VariableAccess access_ = VariableAccess::Read;
protected:
    const MultiPartIdentifier name_;

    explicit VariableRefExpr(const VariableRefExpr &from) : ExprStmt(from), access_{from.access_}, name_{from.name_} {

    };

public:
    VariableRefExpr(source::SourceSpan sourceSpan, const MultiPartIdentifier &name, VariableAccess access = VariableAccess::Read)
        : ExprStmt(sourceSpan), access_{access}, name_{ name } { }

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
        return *new VariableRefExpr(*this);
    }
};


/** Defines a variable and references it. */
class VariableDeclExpr : public VariableRefExpr {
    TypeRef& typeRef_;

private:
    explicit VariableDeclExpr(const VariableDeclExpr &copyFrom) : VariableRefExpr(copyFrom), typeRef_{copyFrom.typeRef_.deepCopyForTemplate()} { }
public:
    VariableDeclExpr(source::SourceSpan sourceSpan, const MultiPartIdentifier &name, TypeRef& typeRef, VariableAccess access = VariableAccess::Read)
        : VariableRefExpr(sourceSpan, name, access),
          typeRef_(typeRef)
    {
    }

    TypeRef &typeRef() { return typeRef_; }
    virtual type::Type &exprType() const override { return typeRef_.type(); }

    virtual bool canWrite() const override { return true; };

    void accept(AstVisitor &visitor) override {
        visitor.visitingVariableDeclExpr(*this);

        if(visitor.shouldVisitChildren()) {
            typeRef_.accept(visitor);
        }

        visitor.visitedVariableDeclExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        VariableDeclExpr &varDecl = *new VariableDeclExpr(*this);
        return varDecl;
    }


};

enum class CastKind : unsigned char {
    Explicit,
    Implicit
};

/** Represents a cast expression... i.e. foo:int= cast<int>(someDouble); */
class CastExpr : public ExprStmt {
    TypeRef &toType_;
    ExprStmt &valueExpr_;
    const CastKind castKind_;

public:
    /** Use this constructor when the type::Type of the cast *is* known in advance. */
    CastExpr(source::SourceSpan sourceSpan, type::Type &toType, ExprStmt& valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toType_(*new KnownTypeRef(sourceSpan, toType)), valueExpr_(valueExpr), castKind_(castKind)
    {
        ASSERT(&toType);
        ASSERT(&valueExpr);
    }

    /** Use this constructor when the type::Type of the cast is *not* known in advance. */
    CastExpr(source::SourceSpan sourceSpan, TypeRef &toType, ExprStmt &valueExpr, CastKind castKind)
        : ExprStmt(sourceSpan), toType_(toType), valueExpr_(valueExpr), castKind_(castKind) { }

    static inline CastExpr &createImplicit(ExprStmt &valueExpr, type::Type &toType) {
        return *new CastExpr(valueExpr.sourceSpan(), *new KnownTypeRef(valueExpr.sourceSpan(), toType), valueExpr, CastKind::Implicit);
    }

    type::Type &exprType() const  override{ return toType_.type(); }
    CastKind castKind() const { return castKind_; }

    virtual bool canWrite() const override { return false; };

    ExprStmt &valueExpr() const { return valueExpr_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingCastExpr(*this);

        if(visitor.shouldVisitChildren()) {
            toType_.accept(visitor);
            valueExpr_.accept(visitor);
        }

        visitor.visitedCastExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new CastExpr(sourceSpan_, toType_.deepCopyForTemplate(), valueExpr_.deepCopyExpandTemplate(expansionContext), castKind_);
    }
};

/** Represents a new expression... i.e. foo:int= new<int>(someDouble); */
class NewExpr : public ExprStmt {
    TypeRef &typeRef_;
public:

    NewExpr(source::SourceSpan sourceSpan, TypeRef &typeRef)
        : ExprStmt(sourceSpan), typeRef_(typeRef)
    {
        ASSERT(&typeRef);
    }

    type::Type &exprType() const  override { return typeRef_.type(); }
    TypeRef &typeRef() const { return typeRef_; }

    virtual bool canWrite() const override { return false; };

    void accept(AstVisitor &visitor) override {
        visitor.visitingNewExpr(*this);

        if(visitor.shouldVisitChildren()) {
            typeRef_.accept(visitor);
        }

        visitor.visitedNewExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &) const override {
        return *new NewExpr(sourceSpan_, typeRef_.deepCopyForTemplate());
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
        visitor.visitingIfExpr(*this);

        if(visitor.shouldVisitChildren()) {
            condition_->accept(visitor);
            thenExpr_->accept(visitor);
            if(elseExpr_)
                elseExpr_->accept(visitor);
        }

        visitor.visitedIfExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new IfExprStmt(
            sourceSpan_,
            condition_->deepCopyExpandTemplate(expansionContext),
            thenExpr_->deepCopyExpandTemplate(expansionContext),
            elseExpr_ ? &elseExpr_->deepCopyExpandTemplate(expansionContext) : nullptr);
    }
};

class WhileExpr : public VoidExprStmt {
    ExprStmt* condition_;
    ExprStmt& body_;
public:

    /** Note:  assumes ownership of condition, truePart and falsePart.  */
    WhileExpr(source::SourceSpan sourceSpan,
              ExprStmt& condition,
              ExprStmt& body)
        : VoidExprStmt(sourceSpan),
          condition_{ &condition },
          body_{ body } {
        ASSERT(&condition_);
        ASSERT(&body_)
    }

    ExprStmt &condition() const { return *condition_; }

    void setCondition(ExprStmt &newCondition) {
        ASSERT(&newCondition);
        condition_ = &newCondition;
    }

    ExprStmt &body() const { return body_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingWhileExpr(*this);

        if(visitor.shouldVisitChildren()) {
            condition_->accept(visitor);
            body_.accept(visitor);
        }

        visitor.visitedWhileExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new WhileExpr(
            sourceSpan_,
            condition_->deepCopyExpandTemplate(expansionContext),
            body_.deepCopyExpandTemplate(expansionContext));
    }
};

class ParameterDef : public AstNode {
    source::SourceSpan span_;
    const Identifier name_;
    TypeRef& typeRef_;
    scope::VariableSymbol *symbol_ = nullptr;
public:
    ParameterDef(source::SourceSpan span, const Identifier &name, TypeRef &typeRef)
        : span_{span}, name_{name}, typeRef_{typeRef} { }

    source::SourceSpan span() { return span_; }
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
            typeRef_.accept(visitor);
        }
        visitor.visitedParameterDef(*this);
    }

    ParameterDef &deepCopy() {
        //I'm not sure that the typeRef_ really needs to be copied.
        return *new ParameterDef(span_, name_, typeRef_.deepCopyForTemplate());
    }
};
type::FunctionType &createFunctionType(type::Type &returnType, const gc_ref_vector<ParameterDef> &parameters);

class FuncDefStmt : public VoidExprStmt {
    const Identifier name_;
    scope::FunctionSymbol *symbol_= nullptr;
    scope::SymbolTable parameterScope_;
    TypeRef &returnTypeRef_;
    gc_ref_vector<ParameterDef> parameters_;
    ExprStmt* body_;
    type::FunctionType &functionType_;

public:
    FuncDefStmt(
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

    const Identifier &name() const { return name_; }
    type::Type &returnType() const { return *functionType_.returnType(); }
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
        visitor.visitingFuncDefStmt(*this);
        if(visitor.shouldVisitChildren()) {
            returnTypeRef_.accept(visitor);
            for(const auto p : parameters_) {
                p.get().accept(visitor);
            }
            body_->accept(visitor);
        }
        visitor.visitedFuncDeclStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ParameterDef> clonedParameters;
        for(ParameterDef &p : parameters_) {
            clonedParameters.emplace_back(p.deepCopy());
        }
        return *new FuncDefStmt(sourceSpan_,
                               name_,
                               returnTypeRef_.deepCopyForTemplate(),
                               clonedParameters,
                                body_->deepCopyExpandTemplate(expansionContext));
    }
};

class MethodRefExpr : public ExprStmt {
    const Identifier name_;
    scope::FunctionSymbol *symbol_ = nullptr;
public:
    explicit MethodRefExpr(const Identifier &name) : ExprStmt(name.span()), name_{ name } {
        ASSERT(name.text().size() > 0);
    }

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
        return *new MethodRefExpr(name_);
    }
};

class FuncCallExpr : public ExprStmt {
    ExprStmt *instanceExpr_;
    source::SourceSpan openParenSpan_;
    ExprStmt &funcExpr_;
    gc_ref_vector<ExprStmt> arguments_;
public:
    FuncCallExpr(
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

    ExprStmt &funcExpr() const { return funcExpr_; }
    ExprStmt *instanceExpr() const { return instanceExpr_; }

    bool canWrite() const override { return false; };

    type::Type &exprType() const override {
        auto &functionType = upcast<type::FunctionType>(funcExpr_.exprType());
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
        visitor.visitingFuncCallExpr(*this);
        if(visitor.shouldVisitChildren()) {
            funcExpr_.accept(visitor);
            if(instanceExpr_) {
                instanceExpr_->accept(visitor);
            }
            for (auto argument : arguments_) {
                argument.get().accept(visitor);
            }
        }
        visitor.visitedFuncCallExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        gc_ref_vector<ExprStmt> clonedArguments;
        clonedArguments.reserve(arguments_.size());
        for(ExprStmt &a : arguments_) {
            clonedArguments.emplace_back(a.deepCopyExpandTemplate(expansionContext));
        }
        return *new FuncCallExpr(sourceSpan_,
                                 (instanceExpr_ ? &instanceExpr_->deepCopyExpandTemplate(expansionContext) : nullptr),
                                openParenSpan_,
                                funcExpr_.deepCopyExpandTemplate(expansionContext),
                                clonedArguments);
    }
};

class NamespaceExpr : public VoidExprStmt {
    MultiPartIdentifier qualifiedName_;
    ExpressionList &body_;
    scope::SymbolTable *scope_;
public:
    NO_COPY_NO_ASSIGN(NamespaceExpr)

    NamespaceExpr(const source::SourceSpan &sourceSpan, const MultiPartIdentifier &qualifiedName, ExpressionList &body)
        : VoidExprStmt(sourceSpan),
          qualifiedName_{qualifiedName}, body_{body} { }

    scope::SymbolTable &scope() {
        ASSERT(scope_);
        return *scope_;
    }

    void setScope(scope::SymbolTable &scope) {
        scope_ = &scope;
    }

    const MultiPartIdentifier &name() { return qualifiedName_; }
    ExpressionList &body() { return body_; }


    void accept(AstVisitor &visitor) override {
        visitor.visitingNamespaceExpr(*this);
        body_.accept(visitor);
        visitor.visitedNamespaceExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new NamespaceExpr(sourceSpan_, qualifiedName_, upcast<ExpressionList>(body_.deepCopyExpandTemplate(expansionContext)));
    }
};

class ClassDefinition : public VoidExprStmt {
    Identifier name_;
    CompoundExpr &body_;
protected:
    ClassDefinition(
        source::SourceSpan span,
        const Identifier &name,
        ast::CompoundExpr &body
    ) : VoidExprStmt{span},
        name_{name},
        body_{body}
    { }
public:
    /** The type of the class being defined. */
    virtual type::Type &definedType() const = 0;

    CompoundExpr &body() const { return body_; }
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

class CompleteClassDefinition : public ClassDefinition {
    gc_ref_vector<TemplateArgument> templateArguments_;
    type::Type &definedType_;

public:
    CompleteClassDefinition(
        source::SourceSpan span,
        const Identifier &name,
        ast::CompoundExpr &body
    ) : ClassDefinition{span, name, body},
        definedType_{*new type::ClassType(AstNode::nodeId(), name.text(), toVectorOfType(templateArguments_))}
    { }

    CompleteClassDefinition(
        source::SourceSpan span,
        const Identifier &name,
        const gc_ref_vector<TemplateArgument> &templateArgs,
        ast::CompoundExpr &body
    ) : ClassDefinition{span, name, body},
        templateArguments_{templateArgs},
        definedType_{*new type::ClassType(AstNode::nodeId(), name.text(), toVectorOfType(templateArguments_))}
    { }

    /** The type of the class being defined. */
    type::Type &definedType() const override { return definedType_; }

    bool hasTemplateArguments() { return !templateArguments_.empty(); }

    void populateClassType() {
        auto &ct = upcast<type::ClassType>(definedType_);

        for (auto &&variable : this->body().scope().variables()) {
            ct.addField(variable.get().name(), variable.get().type());
        }

        for (auto &&method : this->body().scope().functions()) {
            method.get().setThisSymbol(new scope::VariableSymbol("this", this->definedType()));
            ct.addMethod(method.get().name(), method);
        }
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingCompleteClassDefinition(*this);
        if (visitor.shouldVisitChildren()) {
            body().accept(visitor);
        }
        visitor.visitedCompleteClassDefinition(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        //Only once inside an expanded template, a generic class not generic anymore.
        auto &completeClassDef = *new CompleteClassDefinition(
            sourceSpan_,
            name(),
            templateArguments_,
            upcast<CompoundExpr>(body().deepCopyExpandTemplate(expansionContext)));

        return completeClassDef;
    }
};

class GenericClassDefinition : public ClassDefinition {
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
    explicit GenericClassDefinition(const GenericClassDefinition &copyFrom)
        : ClassDefinition(copyFrom),
          templateParameters_{deepCopyVector(copyFrom.templateParameters_)},
          definedType_{copyFrom.definedType_}
    {

    }

public:
    GenericClassDefinition(
        source::SourceSpan span,
        const Identifier &name,
        const gc_ref_vector<TemplateParameter> &templateParameters,
        ast::CompoundExpr &body
    ) : ClassDefinition{span, name, body},
        templateParameters_{templateParameters},
        definedType_{new type::GenericType(nodeId(), name.text(), getTemplateParameterNames(templateParameters))}
    { }

    /** The type of the class being defined. */
    type::Type &definedType() const override { return *definedType_; }

    scope::TypeSymbol *symbol() { ASSERT(symbol_); return symbol_; }
    void setSymbol(scope::TypeSymbol &s) { symbol_ = &s; }

    const gc_ref_vector<TemplateParameter> &templateParameters() {
        return templateParameters_;
    }

    void accept(AstVisitor &visitor) override {
        visitor.visitingGenericClassDefinition(*this);
        if (visitor.shouldVisitChildren()) {
            body().accept(visitor);
        }
        visitor.visitedGenericClassDefinition(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        switch(expansionContext.expansionKind) {
            case ExpansionKind::AnonymousTemplate: {
                auto &completeClassDef = *new CompleteClassDefinition(
                    sourceSpan_,
                    name(),
                    expansionContext.args,
                    upcast<CompoundExpr>(body().deepCopyExpandTemplate(expansionContext)));

                upcast<type::ClassType>(completeClassDef.definedType()).setGenericType(definedType_);
                return completeClassDef;
            }
            case ExpansionKind::NamedTemplate:
                return *new GenericClassDefinition(*this);
            default:
                ASSERT_FAIL("Unhandled ExpansionKind")
        }
        //Only once inside an expanded template, a generic class is not generic anymore so construct CompleteClassDefinition
        auto &completeClassDef = *new CompleteClassDefinition(
            sourceSpan_,
            name(),
            expansionContext.args,
            upcast<CompoundExpr>(body().deepCopyExpandTemplate(expansionContext)));

        upcast<type::ClassType>(completeClassDef.definedType()).setGenericType(definedType_);

        return completeClassDef;
    }
};


class DotExpr : public ExprStmt {
    source::SourceSpan dotSourceSpan_;
    ExprStmt &lValue_;
    const Identifier memberName_;
    type::ClassField *field_ = nullptr;
    bool isWrite_ = false;
public:
    DotExpr(const source::SourceSpan &sourceSpan,
            const source::SourceSpan &dotSourceSpan,
            ExprStmt &lValue,
            const Identifier &memberName)
        : ExprStmt(sourceSpan), dotSourceSpan_{dotSourceSpan}, lValue_{lValue}, memberName_{memberName} { }

    source::SourceSpan dotSourceSpan() { return dotSourceSpan_; };

    bool canWrite() const override { return true; };

    bool isWrite() { return isWrite_; }
    void setIsWrite(bool isWrite) {
        //Eventually, this will need to propagate to the deepest DotExpr, not the first one that's invoked...
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
            lValue_.accept(visitor);
        }
        visitor.visitedDotExpr(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new DotExpr(sourceSpan_, dotSourceSpan_, lValue_.deepCopyExpandTemplate(expansionContext), memberName_);
    }
};

class AssertExprStmt : public VoidExprStmt {
    ast::ExprStmt *condition_;
public:
    AssertExprStmt(const source::SourceSpan &sourceSpan, ExprStmt &condition)
        : VoidExprStmt(sourceSpan), condition_{&condition} {
        ASSERT(condition_);
    }

    ast::ExprStmt &condition() {
        return *condition_;
    }
    void setCondition(ast::ExprStmt &condition) { condition_ = &condition; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingAssertExprStmt(*this);
        if(visitor.shouldVisitChildren()) {
            condition_->accept(visitor);
        }
        visitor.visitedAssertExprStmt(*this);
    }

    ExprStmt &deepCopyExpandTemplate(const TemplateExpansionContext &expansionContext) const override {
        return *new AssertExprStmt(sourceSpan_, condition_->deepCopyExpandTemplate(expansionContext));
    }
};


class Module : public AstNode {
    std::string name_;
    CompoundExpr &body_;
    //gc_unordered_map<std::string, TemplateExprStmt*> templates_;
public:
    Module(const std::string &name, CompoundExpr& body)
        : name_{name}, body_{body} {
    }

    std::string name() const { return name_; }

    scope::SymbolTable &scope() { return body_.scope(); }

    CompoundExpr &body() { return body_; }

    void accept(AstVisitor &visitor) override {
        visitor.visitingModule(*this);
        if(visitor.shouldVisitChildren()) {
            body_.accept(visitor);
        }
        visitor.visitedModule(*this);
    }
};


/**
 * A kind of compilation context for templates, mainly...
 * FIXME: this class really needs a better name.
 */
class AnodeWorld : public gc {
    scope::SymbolTable globalScope_{scope::StorageKind::Global, "::"};
    gc_unordered_map<UniqueId, GenericClassDefinition*> genericClassIndex_;
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

    void addGenericClassDefinition(GenericClassDefinition &genericClass) {
        genericClassIndex_.emplace(genericClass.nodeId(), &genericClass);
    }

    GenericClassDefinition &getGenericClassDefinition(UniqueId nodeId) {
        auto templ = genericClassIndex_[nodeId];
        ASSERT(templ);
        return *templ;
    }

    void addExpandingTemplate(ast::AnonymousTemplateExprStmt &templ) {
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
