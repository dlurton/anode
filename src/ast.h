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
            LiteralBoolExpr,
            LiteralInt32Expr,
            LiteralFloatExpr,
            VariableRefExpr,
            BinaryExpr,
            ConditionalExpr,
            CastExpr,

        };
        std::string to_string(ExprKind kind);

        enum class BinaryOperationKind {
            Assign,
            Add,
            Sub,
            Mul,
            Div,
            Eq,
//            And,
//            Or
        };
        std::string to_string(BinaryOperationKind type);

        bool isArithmeticOperation(BinaryOperationKind kind);
        bool isLogicalOperation(BinaryOperationKind kind);

        class AstNode;
        class Stmt;
        class ReturnStmt;
        class ExprStmt;
        class CompoundStmt;
        class FuncDeclStmt;
        class ReturnStmt;
        class IfExpr;
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

        class AstVisitor {
        public:
            //////////// Statements
            /** Executes before every Stmt is visited. */
            virtual bool visitingStmt(Stmt *) { return true; }
            /** Executes after every Stmt is visited. */
            virtual void visitedStmt(Stmt *) { }

            virtual void visitingFuncDeclStmt(FuncDeclStmt *) { }
            virtual void visitedFuncDeclStmt(FuncDeclStmt *) { }

            virtual void visitingCompoundStmt(CompoundStmt *) { }
            virtual void visitedCompoundStmt(CompoundStmt *) { }

            virtual void visitingReturnStmt(ReturnStmt *) { }
            virtual void visitedReturnStmt(ReturnStmt *) { }

            //////////// Expressions
            /** Executes before every ExprStmt is visited. Return false to prevent visitation of the node's descendants. */
            virtual bool visitingExprStmt(ExprStmt *) { return true; }
            /** Executes after every ExprStmt is visited. */
            virtual void visitedExprStmt(ExprStmt *) { }

            virtual void visitingVariableDeclExpr(VariableDeclExpr *) { }
            virtual void visitedVariableDeclExpr(VariableDeclExpr *) { }

            virtual bool visitingIfExpr(IfExpr *) { }
            virtual void visitedIfExpr(IfExpr *) { }

            virtual void visitingBinaryExpr(BinaryExpr *) { }
            virtual void visitedBinaryExpr(BinaryExpr *) { }

            virtual void visitLiteralBoolExpr(LiteralBoolExpr *) { }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *) { }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *) { }

            virtual void visitVariableRefExpr(VariableRefExpr *) { }

            virtual void visitingCastExpr(CastExpr *) { }
            virtual void visitedCastExpr(CastExpr *) { }

            //////////// Misc
            virtual void visitTypeRef(TypeRef *typeRef) { }

            virtual void visitingModule(Module *) { }
            virtual void visitedModule(Module *) { }
        };

//
//        /** Base class for annotation groups.  Doesn't do much at the moment. */
//        class AnnotationGroup {
//        public:
//            virtual ~AnnotationGroup() { }
//        };
//
//        class AnnotationContainer {
//            //See: http://en.cppreference.com/w/cpp/types/type_info/hash_code
//            using TypeInfoRef = std::reference_wrapper<const std::type_info>;
//            struct Hasher {  std::size_t operator()(TypeInfoRef code) const { return code.get().hash_code(); } };
//            struct EqualTo { bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const { return lhs.get() == rhs.get(); } };
//
//            typedef std::unordered_map<TypeInfoRef, std::unique_ptr<AnnotationGroup>, Hasher, EqualTo> annotation_map;
//            annotation_map annotations_;
//
//        public:
//            template<typename TAnnotation>
//            bool hasAnnotation() {
//                return annotations_.find(typeid(TAnnotation)) == annotations_.end();
//            }
//
//            template<typename TArgs, typename TAnnotation>
//            TAnnotation *newAnnotation(TArgs&& args) {
//                //Remove existing annotation, if any
//                auto itr = annotations_.find(typeid(TAnnotation));
//                if(itr == annotations_.end()) {
//                    annotations_.erase(itr);
//                }
//
//                std::unique_ptr<TAnnotation> annotation = std::make_unique<TAnnotation>(std::forward(args));
//                TAnnotation *annotationPtr = annotation.get();
//                annotations_.emplace(typeid(TAnnotation), std::move(annotation));
//                return annotationPtr;
//           }
//
//            template<typename TAnnotation>
//            TAnnotation* getAnnotation() {
//                auto itr = annotations_.find(typeid(TAnnotation));
//                if(itr == annotations_.end())
//                    return nullptr;
//
//                return (*itr).second;
//            }
//        };

        typedef std::function<std::unique_ptr<ExprStmt>(std::unique_ptr<ExprStmt>)> ExprGraftFunctor;

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

            source::SourceSpan sourceSpan() const {
                return sourceSpan_;
            }
        };

        /** Contains a series of Statments, i.e. those contained within { and }. */
        class CompoundStmt : public Stmt {
            scope::SymbolTable scope_;
        protected:
            std::vector<std::unique_ptr<Stmt>> statements_;
        public:
            CompoundStmt(source::SourceSpan sourceSpan) : Stmt(sourceSpan) { }
            virtual ~CompoundStmt() {}
            virtual StmtKind stmtKind() const override { return StmtKind::CompoundStmt; }
            scope::SymbolTable *scope() { return &scope_; }

            void addStatement(std::unique_ptr<Stmt> stmt) {
                statements_.push_back(std::move(stmt));
            }

            std::vector<Stmt*> statements() const {
                std::vector<Stmt*> retval;
                for(auto &stmt : statements_) {
                    retval.push_back(stmt.get());
                }
                return retval;
            }

            virtual void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingStmt(this);
                visitor->visitingCompoundStmt(this);
                if(visitChildren) {
                    for (auto &stmt : statements_) {
                        stmt->accept(visitor);
                    }
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


        /** Represents a binary expression, i.e. 1 + 2 or foo + bar */
        class BinaryExpr : public ExprStmt {
            std::unique_ptr<ExprStmt> lValue_;
            const BinaryOperationKind operation_;
            const source::SourceSpan operatorSpan_;
            std::unique_ptr<ExprStmt> rValue_;
        public:
            /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
            BinaryExpr(source::SourceSpan sourceSpan,
                       std::unique_ptr<ExprStmt> lValue,
                       BinaryOperationKind operation,
                       source::SourceSpan operatorSpan,
                       std::unique_ptr<ExprStmt> rValue)
                : ExprStmt{sourceSpan},
                  lValue_{std::move(lValue)},
                  operation_{operation},
                  operatorSpan_{operatorSpan},
                  rValue_{move(rValue)} {

                ASSERT(lValue_);
                ASSERT(rValue_);
            }
            virtual ~BinaryExpr() { }

            source::SourceSpan operatorSpan() { return operatorSpan_; }

            ExprKind exprKind() const override { return ExprKind::BinaryExpr; }

            /** This is the type of the result, which may be different than the type of the operands depending on the operation type,
             * because some operation types (e.g. equality, logical and, or, etc) always yield boolean values.  */
            type::Type *type() const override {
                if(operation_ == BinaryOperationKind::Eq /*|| isLogicalOperation(operation_)*/) {
                    return &type::Primitives::Bool;
                }
                return operandsType();
            }

            /** This is the type of the operands. */
            type::Type *operandsType() const {
                ASSERT(rValue_->type() == lValue_->type());
                return rValue_->type();
            }

            ExprStmt *lValue() const { return lValue_.get(); }
            ExprStmt *rValue() const { return rValue_.get(); }

            virtual bool canWrite() const override { return false; };

            void graftLValue(ExprGraftFunctor graftFunctor) {
                lValue_ = std::move(graftFunctor(std::move(lValue_)));
            }

            void graftRValue(ExprGraftFunctor graftFunctor) {
                rValue_ = std::move(graftFunctor(std::move(rValue_)));
            }

            BinaryOperationKind operation() const { return operation_; }

            virtual void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingExprStmt(this);
                visitor->visitingBinaryExpr(this);

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
            VariableAccess setVariableAccess(VariableAccess access) { access_ = access; }

            virtual void accept(AstVisitor *visitor) override {
                visitor->visitingExprStmt(this);
                visitor->visitVariableRefExpr(this);
                visitor->visitedExprStmt(this);
            }
        };


        /** Defines a variable and references it. */
        class VariableDeclExpr : public VariableRefExpr, public scope::Symbol {
            const std::unique_ptr<TypeRef> typeRef_;
        public:
            VariableDeclExpr(source::SourceSpan sourceSpan, const std::string &name,
                             std::unique_ptr<TypeRef> typeRef)
                : VariableRefExpr(sourceSpan, name),
                  typeRef_(std::move(typeRef))
            {
                //I am my own symbol.
                setSymbol(this);
            }

            virtual std::string name() const override { return VariableRefExpr::name(); }

            ExprKind exprKind() const override { return ExprKind::VariableDeclExpr; }
            TypeRef *typeRef() { return typeRef_.get(); }
            virtual type::Type *type() const override { return typeRef_->type(); }

            virtual std::string toString() const override {  return this->name() + ":" + typeRef_->name(); }

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
            std::unique_ptr<TypeRef> toType_;
            const std::unique_ptr<ExprStmt> valueExpr_;
            const CastKind castKind_;
        public:
            /** Use this constructor when the type::Type of the cast *is* known in advance. */
            CastExpr(
                source::SourceSpan sourceSpan,
                type::Type *toType,
                std::unique_ptr<ExprStmt> valueExpr,
                CastKind castKind)
                : ExprStmt(sourceSpan),
                  toType_(std::make_unique<TypeRef>(sourceSpan, toType)),
                  valueExpr_(std::move(valueExpr)),
                  castKind_(castKind) { }

            /** Use this constructor when the type::Type of the cast is *not* known in advance. */
            CastExpr(
                source::SourceSpan sourceSpan,
                std::unique_ptr<TypeRef> toType,
                std::unique_ptr<ExprStmt> valueExpr,
                CastKind castKind)
            : ExprStmt(sourceSpan),
              toType_(std::move(toType)),
              valueExpr_(std::move(valueExpr)),
              castKind_(castKind) { }

            ExprKind exprKind() const override { return ExprKind::CastExpr; }
            type::Type *type() const  override{ return toType_->type(); }
            CastKind castKind() const { return castKind_; }

            virtual bool canWrite() const override { return false; };

            ExprStmt *valueExpr() const { return valueExpr_.get(); }

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

        /** Represents a return statement.  */
        class ReturnStmt : public Stmt {
            const std::unique_ptr<ExprStmt> valueExpr_;
        public:
            ReturnStmt(source::SourceSpan sourceSpan, std::unique_ptr<ExprStmt> valueExpr)
                : Stmt(sourceSpan), valueExpr_(std::move(valueExpr)) {}

            StmtKind stmtKind() const override { return StmtKind::ReturnStmt; }
            const ExprStmt *valueExpr() const { return valueExpr_.get(); }

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
        class IfExpr : public ExprStmt {
            std::unique_ptr<ExprStmt> condition_;
            std::unique_ptr<ExprStmt> truePart_;
            std::unique_ptr<ExprStmt> falsePart_;
        public:

            /** Note:  assumes ownership of condition, truePart and falsePart.  */
            IfExpr(source::SourceSpan sourceSpan,
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

            ExprStmt *condition() const { return condition_.get(); }
            ExprStmt *thenExpr() const { return truePart_.get(); }
            ExprStmt *elseExpr() const { return falsePart_.get(); }

            virtual bool canWrite() const override { return false; };

            void graftCondition(ExprGraftFunctor graftFunctor) {
                condition_= std::move(graftFunctor(std::move(condition_)));
            }

            void graftTruePart(ExprGraftFunctor graftFunctor) {
                truePart_ = std::move(graftFunctor(std::move(truePart_)));
            }

            void graftFalsePart(ExprGraftFunctor graftFunctor) {
                falsePart_ = std::move(graftFunctor(std::move(falsePart_)));
            }


            virtual void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingExprStmt(this);
                visitChildren = visitor->visitingIfExpr(this) ? visitChildren : false;

                if(visitChildren) {
                    condition_->accept(visitor);
                    truePart_->accept(visitor);
                    falsePart_->accept(visitor);
                }

                visitor->visitedIfExpr(this);
                visitor->visitedExprStmt(this);
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
                bool visitChildren = visitor->visitingStmt(this);
                visitor->visitingFuncDeclStmt(this);
                if(visitChildren) {
                    body_->accept(visitor);
                }
                visitor->visitedFuncDeclStmt(this);
                visitor->visitedStmt(this);
            }
        };

        class GlobalExportSymbol : public scope::Symbol {
        private:
            std::string name_;
            type::Type *type_;
        public:
            GlobalExportSymbol(const std::string &name_, type::Type *type_) : name_(name_), type_(type_) { }

            virtual std::string name() const override {
                return name_;
            }

            virtual std::string toString() const override {
                return name_ + ":" + type_->name();
            }

            virtual type::Type *type() const override {
                return type_;
            }
        };

        class Module : public CompoundStmt {
            std::string name_;
        public:
            Module(source::SourceSpan sourceSpan, std::string name)
                : CompoundStmt(sourceSpan), name_{name} {
            }

            std::string name() const { return name_; }

            void accept(AstVisitor *visitor) override {
                bool visitChildren = visitor->visitingStmt(this);
                visitor->visitingModule(this);
                if(visitChildren) {
                    for (auto &stmt : statements_) {
                        stmt->accept(visitor);
                    }
                }
                visitor->visitedModule(this);
                visitor->visitedStmt(this);            }
        };


        /** Most visitors will inherit from this because it tracks symbol scopes through the AST.
         * Note that all overrides in inheritors should the member functions they are overriding so that the scope is properly tracked. */
        // TODO:  move this class back to compile.cpp since it's not being used outside of there anymore
        class ScopeFollowingVisitor : public ast::AstVisitor {
            //We use this only as a stack but it has to be a deque so we can iterate over its contents.
            std::deque<scope::SymbolTable*> symbolTableStack_;
        protected:
            scope::SymbolTable *topScope() {
                ASSERT(symbolTableStack_.size());
                return symbolTableStack_.back();
            }

        public:
            virtual void visitingModule(ast::Module *module) override {
                symbolTableStack_.push_back(module->scope());
            }
            virtual void visitedModule(ast::Module *module) override {
                ASSERT(module->scope() == symbolTableStack_.back())
                symbolTableStack_.pop_back();
            }
            virtual void visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                funcDeclStmt->parameterScope()->setParent(topScope());
                symbolTableStack_.push_back(funcDeclStmt->parameterScope());
            }

            virtual void visitedFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                ASSERT(funcDeclStmt->parameterScope() == symbolTableStack_.back())
                symbolTableStack_.pop_back();
            }

            virtual void visitingCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                compoundStmt->scope()->setParent(topScope());
                symbolTableStack_.push_back(compoundStmt->scope());
            }
            virtual void visitedCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                ASSERT(compoundStmt->scope() == symbolTableStack_.back());
                symbolTableStack_.pop_back();
            }
        };

    } //namespace ast
} //namespace lwnn
