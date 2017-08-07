#pragma once
#pragma once

#include "lwnn.h"

#include "type.h"
#include "scope.h"
#include "source.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <deque>


namespace lwnn {
    namespace ast {

        enum class StmtKind {
            FunctionDeclStmt,
            ReturnStmt,
            ExprStmt,
            CompoundStmt,
            ClassDefinition
        };
        std::string to_string(StmtKind kind);

        enum class ExprKind {
            VariableDeclExpr,
            LiteralBoolExpr,
            LiteralInt32Expr,
            LiteralFloatExpr,
            VariableRefExpr,
            BinaryExpr,
            ConditionalExpr,
            CastExpr,
            CompoundExpr,
        };
        std::string to_string(ExprKind kind);

        enum class BinaryOperationKind {
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
        std::string to_string(BinaryOperationKind type);

        class AstNode;
        class Stmt;
        class ReturnStmt;
        class ExprStmt;
        class CompoundStmt;
        class CompoundExpr;
        class FuncDeclStmt;
        class ClassDefinition;
        class ReturnStmt;
        class IfExprStmt;
        class WhileExpr;
        class LiteralBoolExpr;
        class LiteralInt32Expr;
        class LiteralFloatExpr;
        class BinaryExpr;
        class VariableDeclExpr;
        class VariableRefExpr;
        class AssignExpr;
        class CastExpr;
        class TypeRef;
        class Module;

        class AstVisitor : public gc {
        public:
            //////////// Statements
            /** Executes before every Stmt is visited. */
            virtual bool visitingStmt(Stmt *) { return true; }
            /** Executes after every Stmt is visited. */
            virtual void visitedStmt(Stmt *) { }

            virtual void visitingFuncDeclStmt(FuncDeclStmt *) { }
            virtual void visitedFuncDeclStmt(FuncDeclStmt *) { }

            virtual bool visitingClassDefinition(ClassDefinition *) { return true; }
            virtual void visitedClassDefinition(ClassDefinition *) {  }

            virtual void visitingReturnStmt(ReturnStmt *) { }
            virtual void visitedReturnStmt(ReturnStmt *) { }

            virtual bool visitingCompoundStmt(CompoundStmt *) { return true;}
            virtual void visitedCompoundStmt(CompoundStmt*) { }

            //////////// Expressions
            /** Executes before every ExprStmt is visited. Return false to prevent visitation of the node's descendants. */
            virtual bool visitingExprStmt(ExprStmt *) { return true; }
            /** Executes after every ExprStmt is visited. */
            virtual void visitedExprStmt(ExprStmt *) { }

            virtual void visitingVariableDeclExpr(VariableDeclExpr *) { }
            virtual void visitedVariableDeclExpr(VariableDeclExpr *) { }

            virtual bool visitingIfExpr(IfExprStmt *) { return true; }
            virtual void visitedIfExpr(IfExprStmt *) { }

            virtual bool visitingWhileExpr(WhileExpr *) { return true; }
            virtual void visitedWhileExpr(WhileExpr *) { }

            virtual bool visitingBinaryExpr(BinaryExpr *) { return true; }
            virtual void visitedBinaryExpr(BinaryExpr *) { }

            virtual void visitLiteralBoolExpr(LiteralBoolExpr *) { }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *) { }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *) { }

            virtual void visitVariableRefExpr(VariableRefExpr *) { }

            virtual void visitingCastExpr(CastExpr *) { }
            virtual void visitedCastExpr(CastExpr *) { }

            virtual bool visitingCompoundExpr(CompoundExpr *) { return true;}
            virtual void visitedCompoundExpr(CompoundExpr *) { }

            //////////// Misc
            virtual void visitTypeRef(TypeRef *) { }

            virtual bool visitingModule(Module *) { return true; }
            virtual void visitedModule(Module *) { }

        };

        typedef std::function<ExprStmt* (ExprStmt*)> ExprGraftFunctor;

        /** Base class for all nodes */
        class AstNode : public gc {
        public:
            virtual ~AstNode() {}
            virtual void accept(AstVisitor *visitor) = 0;
        };

        /** A statement any kind of non-terminal that may appear within a CompountStatement (i.e. between { and }), or
         * at the global scope, i.e. global variables, assignments, class definitions,
         */
        class Stmt : public AstNode {
        protected:
            source::SourceSpan sourceSpan_;
            Stmt(source::SourceSpan sourceSpan) : sourceSpan_(sourceSpan) { }
        public:
            virtual StmtKind stmtKind() const = 0;

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
            type::Type *referencedType_;
            std::vector<scope::DelayedTypeResolutionSymbol*> symbolsToNotify_;
        public:
            /** Constructor to be used when the data type isn't known yet and needs to be resolved later. */
            TypeRef(source::SourceSpan sourceSpan, std::string name)
                : sourceSpan_(sourceSpan), name_(name), referencedType_(nullptr) { }

            /** Constructor to be used when the data type is known and doesn't need to be resolved. */
            TypeRef(source::SourceSpan sourceSpan, type::Type *dataType)
                : sourceSpan_(sourceSpan), name_(dataType->name()), referencedType_(dataType) { }

            source::SourceSpan sourceSpan() {
                return sourceSpan_;
            }

            std::string name() const { return name_; }

            type::Type *type() const {
                ASSERT(referencedType_ && "Data type hasn't been resolved yet");
                return referencedType_;
            }

            void setType(type::Type *referencedType) {
                referencedType_ = referencedType;
                for(auto symbol : symbolsToNotify_)
                    symbol->setType(referencedType_);
            }

            void addSymbolToNotify(scope::DelayedTypeResolutionSymbol *symbol) {
                symbolsToNotify_.push_back(symbol);
            }

            virtual void accept(AstVisitor *visitor) override {
                visitor->visitTypeRef(this);
            }
        };


        /** Base class for all expressions. */
        class ExprStmt : public Stmt {
        protected:
            ExprStmt(source::SourceSpan sourceSpan) : Stmt(sourceSpan) { }
        public:
            virtual ~ExprStmt() { }
            virtual StmtKind stmtKind() const override { return StmtKind::ExprStmt; }
            virtual ExprKind exprKind() const = 0;
            virtual type::Type *type() const  = 0;
            virtual bool canWrite() const = 0;
        };


        /** Represents a literal boolean */
        class LiteralBoolExpr : public ExprStmt {
            bool const value_;
        public:
            LiteralBoolExpr(source::SourceSpan sourceSpan, const bool value) : ExprStmt(sourceSpan), value_(value) {}
            virtual ~LiteralBoolExpr() {}
            virtual ExprKind exprKind() const override { return ExprKind::LiteralBoolExpr; }
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
            virtual ExprKind exprKind() const override { return ExprKind::LiteralInt32Expr; }
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
            ExprKind exprKind() const override { return ExprKind::LiteralFloatExpr; }
            type::Type *type() const override {  return &type::Primitives::Float; }
            float value() const { return value_; }

            virtual bool canWrite() const override { return false; };


            virtual void accept(AstVisitor *visitor) override {
                visitor->visitingExprStmt(this);
                visitor->visitLiteralFloatExpr(this);
                visitor->visitedExprStmt(this);
            }
        };


        /** Represents the type of binary operation.
         * Distinction between these binary operation types are important because although they
         * use the same data structure to represent them, how their code is emitted is very
         * different.
         */
        enum class BinaryExprKind {
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

            ExprKind exprKind() const override { return ExprKind::BinaryExpr; }

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
                ASSERT(rValue_->type() == lValue_->type());
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

        enum class VariableAccess {
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
            ExprKind exprKind() const override { return ExprKind::VariableRefExpr; }

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

            ExprKind exprKind() const override { return ExprKind::VariableDeclExpr; }
            TypeRef *typeRef() { return typeRef_; }
            virtual type::Type *type() const override { return typeRef_->type(); }

            //virtual std::string toString() const override {  return this->name() + ":" + typeRef_->name(); }

            virtual bool canWrite() const override { return false; };

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

        enum class CastKind {
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
            static CastExpr *createImplicit(ExprStmt* valueExpr, ast::TypeRef *toType);

            ExprKind exprKind() const override { return ExprKind::CastExpr; }
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
            std::vector<ExprStmt*> expressions_;
        public:
            CompoundExpr(source::SourceSpan sourceSpan) : ExprStmt(sourceSpan) { }
            virtual ~CompoundExpr() {}
            scope::SymbolTable *scope() { return &scope_; }

            virtual ExprKind exprKind() const { return ExprKind::CompoundExpr; };
            virtual type::Type *type() const {
                ASSERT(expressions_.size() > 0);
                return expressions_.back()->type();
            };
            virtual bool canWrite() const { return false; };

            void addExpr(ExprStmt* stmt) {
                expressions_.push_back(stmt);
            }

            std::vector<ExprStmt*> expressions() const {
                std::vector<ExprStmt*> retval;
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

        /** Contains a series of expressions, i.e. those contained within { and }. */
        class CompoundStmt : public Stmt {
            scope::SymbolTable scope_;
        protected:
            std::vector<Stmt*> statements_;
        public:
            CompoundStmt(source::SourceSpan sourceSpan) : Stmt(sourceSpan) { }
            virtual ~CompoundStmt() {}

            StmtKind stmtKind() const override {
                return StmtKind::CompoundStmt;
            }

            scope::SymbolTable *scope() { return &scope_; }

            void addStmt(Stmt* stmt) {
                statements_.push_back(stmt);
            }

            std::vector<Stmt*> statements() const {
                std::vector<Stmt*> retval;
                retval.reserve(statements_.size());
                for(auto &stmt : statements_) {
                    retval.push_back(stmt);
                }
                return retval;
            }

            virtual void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingStmt(this);
                visitChildren = visitor->visitingCompoundStmt(this) ? visitChildren : false;
                if(visitChildren) {
                    for (auto &stmt : statements_) {
                        stmt->accept(visitor);
                    }
                }
                visitor->visitedCompoundStmt(this);
                visitor->visitedStmt(this);
            }
        };


        /** Represents a return statement.  */
        class ReturnStmt : public Stmt {
            ExprStmt* valueExpr_;
        public:
            ReturnStmt(source::SourceSpan sourceSpan, ExprStmt* valueExpr)
                : Stmt(sourceSpan), valueExpr_(valueExpr) {}

            StmtKind stmtKind() const override { return StmtKind::ReturnStmt; }
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

            virtual ExprKind exprKind() const override {  return ExprKind::ConditionalExpr; }

            type::Type *type() const override {
                if(elseExpr_ == nullptr || thenExpr_->type() != elseExpr_->type()) {
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

            virtual ExprKind exprKind() const override {  return ExprKind::ConditionalExpr; }

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

        class FuncDeclStmt : public Stmt {
            const std::string name_;
            TypeRef* returnTypeRef_;
            scope::SymbolTable* parameterScope_;
            Stmt* body_;

        public:
            FuncDeclStmt(source::SourceSpan sourceSpan,
                     std::string name,
                     TypeRef* returnTypeRef,
                     Stmt* body)
                : Stmt(sourceSpan),
                  name_{ name },
                  returnTypeRef_{ returnTypeRef },
                  body_{ body } { }

            StmtKind stmtKind() const override { return StmtKind::FunctionDeclStmt; }
            std::string name() const { return name_; }
            TypeRef *returnTypeRef() const { return returnTypeRef_; }
            scope::SymbolTable *parameterScope() const { return parameterScope_; };
            Stmt *body() const { return body_; }

            virtual void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingStmt(this);
                visitor->visitingFuncDeclStmt(this);
                if(visitChildren) {
                    body_->accept(visitor);
                }
                visitor->visitedFuncDeclStmt(this);
                visitor->visitedStmt(this);
            }
        };

        class ClassDefinition : public Stmt {
            scope::SymbolTable scope_;
            std::string name_;
            CompoundStmt *body_;

            type::ClassType *classType_;
        public:
            ClassDefinition(source::SourceSpan span, std::string name, CompoundStmt *body)
                : Stmt{span}, name_{name}, body_{body}, classType_{new type::ClassType(name)} { }

            StmtKind stmtKind() const override { return StmtKind::ClassDefinition; }

            scope::SymbolTable *scope() {
                return &scope_;
            }

            CompoundStmt *body() { return body_; }

            std::string name() { return name_; }

            type::ClassType *classType() { return classType_; }

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

        class Module : public gc {
            std::string name_;
            CompoundStmt *body_;
        public:
            Module(std::string name, CompoundStmt* body)
                : name_{name}, body_{body} {
            }

            std::string name() const { return name_; }

            scope::SymbolTable *scope() { return body_->scope(); }

            CompoundStmt *body() { return body_; }

            void accept(AstVisitor *visitor) {
                if(visitor->visitingModule(this)) {
                    body_->accept(visitor);
                }
                visitor->visitedModule(this);
            }
        };

    } //namespace ast
} //namespace lwnn
