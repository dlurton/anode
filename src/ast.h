#pragma once

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
            CompoundStmt,
            FunctionDeclStmt,
            ReturnStmt,
            ExprStmt
        };
        std::string to_string(StmtKind kind);

        enum class ExprKind {
            VariableDeclExpr,
            LiteralInt32Expr,
            LiteralFloatExpr,
            VariableRefExpr,
            AssignVariableExpr,
            BinaryExpr,
            ConditionalExpr,
        };
        std::string to_string(ExprKind kind);

        enum class BinaryOperationKind {
            Add,
            Sub,
            Mul,
            Div
        };
        std::string to_string(BinaryOperationKind type);


        class AstNode;
        class Stmt;
        class ReturnStmt;
        class ExprStmt;
        class CompoundStmt;
        class FuncDeclStmt;
        class ReturnStmt;
        class ConditionalExpr;
        class LiteralInt32Expr;
        class LiteralFloatExpr;
        class BinaryExpr;
        class VariableDeclExpr;
        class VariableRefExpr;
        class AssignExpr;
        class TypeRef;
        class Module;


        /** Visitor part of visitor pattern, with some enhancements.
         *
         *  AstNodes with no children follow a simple convention:
         *      void visit[AstNodeType](AstNodeType *)
         *  i.e.
         *      void visitLiteralInt32Expr(LiteralInt32Expr *expr)
         *
         *  AstNodes with children have have two methods each:
         *      bool visiting[AstNodeType](AstNodeType *)
         *      void visited[AstNodeType](AstNOdeType *)
         * i.e.
         *      virtual bool visitingBinaryExpr(BinaryExpr *) {}
         *      virtual void visitedBinaryExpr(BinaryExpr *) {}
         *
         * Functions prefixed with "visiting" are executed before their descendants are visited.  This type of method
         * also returns a bool--implementers of this method should return false to prevent visitation of their
         * descendants.
         *
         * Functions prefixed with "visited" are executed after their descendants are visited.
         */
        class AstVisitor {
        public:
            /** Executes before every Stmt is visited. Return false to prevent visitation of the node's descendants. */
            virtual bool visitingStmt(Stmt *) { return true; }
            /** Executes after every Stmt is visited. */
            virtual void visitedStmt(Stmt *) {}

            /** Executes before every ExprStmt is visited. */
            virtual bool visitingExprStmt(ExprStmt *) { return true; }
            /** Executes after every ExprStmt is visited. */
            virtual void visitedExprStmt(ExprStmt *) {}


            virtual bool visitingModule(Module *) { return true; }
            virtual void visitedModule(Module *) {}

            virtual void visitTypeRef(TypeRef *typeRef) {  }

            //////////// Statements
            virtual bool visitingFuncDeclStmt(FuncDeclStmt *) { return true; }
            virtual void visitedFuncDeclStmt(FuncDeclStmt *) {}

            virtual bool visitingCompoundStmt(CompoundStmt *) { return true; }
            virtual void visitedCompoundStmt(CompoundStmt *) {}

            virtual bool visitingReturnStmt(ReturnStmt *) { return true; }
            virtual void visitedReturnStmt(ReturnStmt *) {}

            //////////// Expressions
            virtual bool visitingVariableDeclExpr(VariableDeclExpr *) { return true; }
            virtual void visitedVariableDeclExpr(VariableDeclExpr *) {}

            virtual bool visitingConditionalExpr(ConditionalExpr *) { return true; }
            virtual void visitedConditionalExpr(ConditionalExpr *) {}

            virtual bool visitingBinaryExpr(BinaryExpr *) { return true; }
            virtual void visitedBinaryExpr(BinaryExpr *) {}

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *) { }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *) {}

            virtual void visitVariableRefExpr(VariableRefExpr *) {}

            virtual bool visitingAssignExpr(AssignExpr *) { return true; }
            virtual void visitedAssignExpr(AssignExpr *) {}
        };


        /** Base class for annotation groups.  Doesn't do much at the moment. */
        class AnnotationGroup {
        public:
            virtual ~AnnotationGroup() { }
        };

        class AnnotationContainer {
            //See: http://en.cppreference.com/w/cpp/types/type_info/hash_code
            using TypeInfoRef = std::reference_wrapper<const std::type_info>;
            struct Hasher {  std::size_t operator()(TypeInfoRef code) const { return code.get().hash_code(); } };
            struct EqualTo { bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const { return lhs.get() == rhs.get(); } };

            typedef std::unordered_map<TypeInfoRef, std::unique_ptr<AnnotationGroup>, Hasher, EqualTo> annotation_map;
            annotation_map annotations_;

        public:
            template<typename TAnnotation>
            bool hasAnnotation() {
                return annotations_.find(typeid(TAnnotation)) == annotations_.end();
            }

            template<typename TArgs, typename TAnnotation>
            TAnnotation *newAnnotation(TArgs&& args) {
                //Remove existing annotation, if any
                auto itr = annotations_.find(typeid(TAnnotation));
                if(itr == annotations_.end()) {
                    annotations_.erase(itr);
                }

                std::unique_ptr<TAnnotation> annotation = std::make_unique<TAnnotation>(std::forward(args));
                TAnnotation *annotationPtr = annotation.get();
                annotations_.emplace(typeid(TAnnotation), std::move(annotation));
                return annotationPtr;
           }

            template<typename TAnnotation>
            TAnnotation* getAnnotation() {
                auto itr = annotations_.find(typeid(TAnnotation));
                if(itr == annotations_.end())
                    return nullptr;

                return (*itr).second;
            }
        };

        /** Base class for all nodes */
        class AstNode {
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

            source::SourceSpan sourceSpan() {
                return sourceSpan_;
            }
        };

        /** Contains a series of Statments, i.e. those contained within { and }. */
        class CompoundStmt : public Stmt {
            std::vector<std::unique_ptr<Stmt>> statements_;
            scope::SymbolTable scope_;
        public:
            CompoundStmt(source::SourceSpan sourceSpan) : Stmt(sourceSpan) { }
            virtual ~CompoundStmt() {}
            virtual StmtKind stmtKind() const override { return StmtKind::CompoundStmt; }
            scope::SymbolTable *scope() { return &scope_; }

            void addStatement(std::unique_ptr<Stmt> stmt) {
                statements_.push_back(std::move(stmt));
            }

            typedef std::vector<Stmt*> child_iterator;

            std::vector<Stmt*> statements() const {
                std::vector<Stmt*> retval;
                for(auto &stmt : statements_) {
                    retval.push_back(stmt.get());
                }
                return retval;
            }

            virtual void accept(AstVisitor *visitor) override {
                if(!visitor->visitingStmt(this) && visitor->visitingCompoundStmt(this)) {
                    return;
                }
                for(auto &stmt : statements_) {
                    stmt->accept(visitor);
                }
                visitor->visitedCompoundStmt(this);
                visitor->visitedStmt(this);
            }
        };


        /* Refers to a data type, i.e. "int", or "WidgetFactory."
         * Initially, type references will be unresolved (i.e. referencedType_ is null) but this is resolved
         * during an AST pass. */
        class TypeRef : AstNode {
            source::SourceSpan sourceSpan_;
            const std::string name_;
            type::Type *referencedType_;
        public:
            /** Constructor to be used when the data type isn't known yet and needs to be resolved later. */
            TypeRef(source::SourceSpan sourceSpan, std::string name)
                : sourceSpan_(sourceSpan), name_(name) { }

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
            virtual void accept(AstVisitor *visitor) override {
                if(!visitor->visitingExprStmt(this)) {
                    return;
                }
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

            virtual void accept(AstVisitor *visitor) override {
                if(!visitor->visitingExprStmt(this)) {
                    return;
                }
                visitor->visitLiteralFloatExpr(this);
                visitor->visitedExprStmt(this);
            }
        };

        /** Defines a variable. */
        class VariableDeclExpr :
                public ExprStmt,
                public scope::Symbol {

            const std::string name_;
            const std::unique_ptr<TypeRef> typeRef_;
            const std::unique_ptr<ExprStmt> initializerExpr_;

        public:
            VariableDeclExpr(source::SourceSpan sourceSpan, const std::string &name,
                             std::unique_ptr<TypeRef> typeRef, std::unique_ptr<ExprStmt> initializerExpr)
                : ExprStmt(sourceSpan),
                  name_(name),
                  typeRef_(std::move(typeRef)),
                  initializerExpr_(std::move(initializerExpr))
            {
                ASSERT(name.size() > 0);
            }

            ExprKind exprKind() const override { return ExprKind::VariableDeclExpr; }
            TypeRef *typeRef() { return typeRef_.get(); }
            virtual type::Type *type() const override { return typeRef_->type(); }
            ExprStmt *initializerExpr() { return initializerExpr_.get(); }

            virtual std::string name() const override { return name_; }
            virtual std::string toString() const override {  return name_ + ":" + typeRef_->name(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingExprStmt(this) &&  visitor->visitingVariableDeclExpr(this))) {
                    return;
                }
                typeRef_->accept(visitor);
                if(initializerExpr_) {
                    initializerExpr_->accept(visitor);
                }
                visitor->visitedVariableDeclExpr(this);
                visitor->visitedExprStmt(this);
            }
        };

        /** Represents a binary expression, i.e. 1 + 2 or foo + bar */
        class BinaryExpr : public ExprStmt {
            const std::unique_ptr<ExprStmt> lValue_;
            const BinaryOperationKind operation_;
            const std::unique_ptr<ExprStmt> rValue_;
        public:
            /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
            BinaryExpr(source::SourceSpan sourceSpan, std::unique_ptr<ExprStmt> lValue, BinaryOperationKind operation,
                       std::unique_ptr<ExprStmt> rValue)
                : ExprStmt(sourceSpan), lValue_(std::move(lValue)), operation_(operation), rValue_(move(rValue)) {
                ASSERT(lValue_);
                ASSERT(rValue_);
            }
            virtual ~BinaryExpr() { }
            ExprKind exprKind() const override { return ExprKind::BinaryExpr; }
            /** The data type of an rValue expression is always the same as the rValue's data type. */
            type::Type *type() const override { return rValue_->type(); }
            ExprStmt *lValue() const { return lValue_.get(); }
            ExprStmt *rValue() const { return rValue_.get(); }
            BinaryOperationKind operation() const { return operation_; }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingExprStmt(this) && visitor->visitingBinaryExpr(this))) {
                    return;
                }
                lValue_->accept(visitor);
                rValue_->accept(visitor);
                visitor->visitedBinaryExpr(this);
                visitor->visitedExprStmt(this);
            }
        };

        /** Represents a reference to a previously declared variable. */
        class VariableRefExpr : public ExprStmt {
            std::string name_;
            std::shared_ptr<scope::Symbol> symbol_;
        public:
            VariableRefExpr(source::SourceSpan sourceSpan, std::string name) : ExprStmt(sourceSpan), name_{ name } { }
            ExprKind exprKind() const override { return ExprKind::VariableRefExpr; }

            virtual type::Type *type() const override {
                ASSERT(symbol_);
                return symbol_->type();
            }

            std::string name() const { return name_; }
            std::string toString() const { return name_ + ":" + this->type()->name(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!visitor->visitingStmt(this)) {
                    return;
                }
                visitor->visitVariableRefExpr(this);
                visitor->visitedExprStmt(this);
            }
        };

        /** Represents an assignment expression. */
        class AssignExpr : public ExprStmt, public AnnotationContainer {
            std::unique_ptr<VariableRefExpr> assignee_;
            const std::unique_ptr<ExprStmt> valueExpr_;
        public:
            AssignExpr(source::SourceSpan sourceSpan,
                       std::unique_ptr<VariableRefExpr> variable,
                       std::unique_ptr<ExprStmt> valueExpr)
                : ExprStmt(sourceSpan), assignee_(std::move(variable)), valueExpr_(std::move(valueExpr)) { }

            ExprKind exprKind() const override { return ExprKind::AssignVariableExpr; }
            ExprStmt *valueExpr() const { return valueExpr_.get(); }
            VariableRefExpr *assignee() { return assignee_.get(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingExprStmt(this) && visitor->visitingAssignExpr(this))) {
                    return;
                }
                assignee_->accept(visitor);
                valueExpr_->accept(visitor);
                visitor->visitedAssignExpr(this);
                visitor->visitingExprStmt(this);
            }
        };

        /** Represents a return statement.  */
        class ReturnStmt : public Stmt {
            const std::unique_ptr<ExprStmt> valueExpr_;
        public:
            ReturnStmt(source::SourceSpan sourceSpan, std::unique_ptr<ExprStmt> valueExpr)
                : Stmt(sourceSpan), valueExpr_(std::move(valueExpr)) {}

            StmtKind stmtKind() const override { return StmtKind::ReturnStmt; }
            const ExprStmt *valueExpr() const { return valueExpr_.get(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingStmt(this) && visitor->visitingReturnStmt(this))) {
                    return;
                }
                valueExpr_->accept(visitor);
                visitor->visitedReturnStmt(this);
                visitor->visitedStmt(this);
            }
        };

        /** Can be the basis of an if-then-else or ternary operator. */
        class ConditionalExpr : public ExprStmt {
            std::unique_ptr<ExprStmt> condition_;
            std::unique_ptr<ExprStmt> truePart_;
            std::unique_ptr<ExprStmt> falsePart_;
        public:

            /** Note:  assumes ownership of condition, truePart and falsePart.  */
            ConditionalExpr(source::SourceSpan sourceSpan,
                            std::unique_ptr<ExprStmt> condition,
                            std::unique_ptr<ExprStmt> truePart,
                            std::unique_ptr<ExprStmt> falsePart)
                : ExprStmt(sourceSpan),
                  condition_{ std::move(condition) },
                  truePart_{ std::move(truePart) },
                  falsePart_{ std::move(falsePart) } {
                ASSERT(condition_);
            }

            virtual ExprKind exprKind() const override {  return ExprKind::ConditionalExpr; }
            type::Type *type() const override {
                if (truePart_ != nullptr) {
                    return truePart_->type();
                } else {
                    if (falsePart_ == nullptr) {
                        return &type::Primitives::Void;
                    } else {
                        return falsePart_->type();
                    }
                }
            }

            const ExprStmt *condition() const { return condition_.get(); }
            const ExprStmt *truePart() const { return truePart_.get(); }
            const ExprStmt *falsePart() const { return falsePart_.get(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingExprStmt(this) && visitor->visitingConditionalExpr(this))) {
                    return;
                }
                truePart_->accept(visitor);
                visitor->visitedConditionalExpr(this);
                visitor->visitingExprStmt(this);
            }
        };

        class FuncDeclStmt : public Stmt {
            const std::string name_;
            std::unique_ptr<TypeRef> returnTypeRef_;
            std::unique_ptr<scope::SymbolTable> parameterScope_;
            std::unique_ptr<Stmt> body_;

        public:
            FuncDeclStmt(source::SourceSpan sourceSpan,
                     std::string name,
                     std::unique_ptr<TypeRef> returnTypeRef,
                     std::unique_ptr<Stmt> body)
                : Stmt(sourceSpan),
                  name_{ name },
                  returnTypeRef_{ std::move(returnTypeRef) },
                  body_{ std::move(body) } { }

            StmtKind stmtKind() const override { return StmtKind::FunctionDeclStmt; }
            std::string name() const { return name_; }
            TypeRef *returnTypeRef() const { return returnTypeRef_.get(); }
            scope::SymbolTable *parameterScope() const { return parameterScope_.get(); };
            Stmt *body() const { return body_.get(); }

            virtual void accept(AstVisitor *visitor) override {
                if(!(visitor->visitingStmt(this) && visitor->visitingFuncDeclStmt(this))) {
                    return;
                }
                body_->accept(visitor);
                visitor->visitedFuncDeclStmt(this);
                visitor->visitingStmt(this);
            }
        };

        class Module : public AstNode {
            const std::string name_;
            std::unique_ptr<Stmt> body_;
            scope::SymbolTable scope_;
        public:
            Module(std::string name, std::unique_ptr<Stmt> body)
                : name_{name}, body_{std::move(body)} {
                ASSERT(body_);
            }

            std::string name() const { return name_; }
            Stmt *body() const { return body_.get(); }
            scope::SymbolTable *scope() { return &scope_; }

            void accept(AstVisitor *visitor) override {
                if(!visitor->visitingModule(this)) {
                    return;
                }
                body_->accept(visitor);
                visitor->visitedModule(this);
            }
        };


        /** Most visitors will inherit from this since tracks symbol scopes through the AST. */
        class ScopeFollowingVisitor : public ast::AstVisitor {
            //We use this only as a stack but it has to be a deque so we can iterate over its contents.
            std::deque<scope::SymbolTable*> symbolTableStack_;
        protected:
            scope::SymbolTable *topScope() {
                ASSERT(symbolTableStack_.size());
                return symbolTableStack_.back();
            }

        public:
            virtual bool visitingModule(ast::Module *module) override {
                symbolTableStack_.push_back(module->scope());
                return true;
            }
            virtual void visitedModule(ast::Module *module) override {
                ASSERT(module->scope() == symbolTableStack_.back())
                symbolTableStack_.pop_back();
            }

            virtual bool visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                funcDeclStmt->parameterScope()->setParent(topScope());
                symbolTableStack_.push_back(funcDeclStmt->parameterScope());
                return true;
            }
            virtual void visitedFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                ASSERT(funcDeclStmt->parameterScope() == symbolTableStack_.back())
                symbolTableStack_.pop_back();
            }

            virtual bool visitingCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                compoundStmt->scope()->setParent(topScope());
                symbolTableStack_.push_back(compoundStmt->scope());
                return true;
            }
            virtual void visitedCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                ASSERT(compoundStmt->scope() == symbolTableStack_.back());
                symbolTableStack_.pop_back();
            }
        };

    } //namespace ast
} //namespace lwnn
