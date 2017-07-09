#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "exception.h"

/** Rules for AST nodes:
 *      - Every node shall "own" its child nodes so that when a node is deleted, all of its children are also deleted.
 *      - Thou shalt not modify any AST node after it has been created.
 */

namespace lwnn {
    namespace ast {
        using std::make_unique;
        using std::make_shared;
        using std::move;

        enum class NodeKind {
            Binary,
            Invoke,
            VariableRef,
            Conditional,
            Switch,
            Block,
            LiteralInt32,
            LiteralFloat,
            AssignVariable,
            Return,
            Function
        };

        std::string to_string(NodeKind nodeKind);

        enum class OperationKind {
            Add,
            Sub,
            Mul,
            Div
        };

        std::string to_string(OperationKind type);

        enum class DataType {
            Void,
            Bool,
            Int32,
            Pointer,
            Float,
            Double
        };

        std::string to_string(DataType dataType);

        /** Represents a location within a source file. */
        struct SourceLocation {
            const size_t line;
            const size_t position;

            SourceLocation(size_t position, size_t column)
                : line(position), position(column) {}
        };

        /** Represents a range within a source defined by a starting SourceLocation and and ending SourceLocation.*/
        struct SourceSpan {
            /** The name of the input i.e. a filename, "stdin" or "REPL". */
            const std::string name;  //TODO:  I feel like it's horribly inefficient to be copying this around everywhere...
            const SourceLocation start;
            const SourceLocation end;

            SourceSpan(std::string sourceName, SourceLocation start, SourceLocation end)
                : name(sourceName), start(start), end(end) {}

            static SourceSpan Any;
        };

//    /** Specifies a string from the source code and its location. */
//    struct Identifier {
//        const std::string name;
//        const SourceSpan sourceSpan;
//        Identifier(std::string name, SourceSpan sourceSpan)
//            : name(name), sourceSpan{span) {  }
//    };

        /** Base class for all nodes */
        class AstNode {
        private:
            SourceSpan sourceSpan_;
        public:
            AstNode(SourceSpan sourceSpan) : sourceSpan_(sourceSpan) {}

            virtual ~AstNode() {}

            virtual NodeKind nodeKind() const = 0;
        };

        /** Base class for all expressions. */
        class Expr : public AstNode {
        public:
            Expr(SourceSpan sourceSpan) : AstNode(sourceSpan) {}

            virtual ~Expr() {}

            virtual DataType dataType() const { return DataType::Void; }
        };

        /** Represents an expression that is a literal 32 bit integer. */
        class LiteralInt32Expr : public Expr {
            int const value_;
        public:
            LiteralInt32Expr(SourceSpan sourceSpan, const int value) : Expr(sourceSpan), value_(value) {}

            virtual ~LiteralInt32Expr() {}

            NodeKind nodeKind() const override { return NodeKind::LiteralInt32; }

            DataType dataType() const override {
                return DataType::Int32;
            }

            int value() const {
                return value_;
            }
        };


        /** Represents an expression that is a literal float. */
        class LiteralFloatExpr : public Expr {
            float const value_;
        public:
            LiteralFloatExpr(SourceSpan sourceSpan, const float value) : Expr(sourceSpan), value_(value) {}

            virtual ~LiteralFloatExpr() {}

            NodeKind nodeKind() const override {
                return NodeKind::LiteralFloat;
            }

            DataType dataType() const override {
                return DataType::Float;
            }

            float value() const {
                return value_;
            }
        };

        /** Represents a binary expression, i.e. 1 + 2 or foo + bar */
        class BinaryExpr : public Expr {
            const std::unique_ptr<const Expr> lValue_;
            const OperationKind operation_;
            const std::unique_ptr<const Expr> rValue_;
        public:

            /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
            BinaryExpr(SourceSpan sourceSpan, std::unique_ptr<const Expr> lValue, OperationKind operation,
                       std::unique_ptr<const Expr> rValue)
                : Expr(sourceSpan), lValue_(std::move(lValue)), operation_(operation), rValue_(std::move(rValue)) {
                ASSERT_NOT_NULL(lValue_);
                ASSERT_NOT_NULL(rValue_);
            }

            virtual ~BinaryExpr() {}

            NodeKind nodeKind() const override {
                return NodeKind::Binary;
            }

            /** The data type of an rValue expression is always the same as the rValue's data type. */
            DataType dataType() const override {
                return rValue_->dataType();
            }

            const Expr *lValue() const {
                return lValue_.get();
            }

            const Expr *rValue() const {
                return rValue_.get();
            }

            OperationKind operation() const {
                return operation_;
            }
        };


        /** Defines a variable or a variable reference. */
        class VariableDef : AstNode {
            const std::string name_;
            const DataType dataType_;

        public:
            VariableDef(SourceSpan sourceSpan, const std::string &name_, const DataType dataType)
                : AstNode(sourceSpan), name_(name_), dataType_(dataType) {
                DEBUG_ASSERT(name.size() > 0);
            }

            DataType dataType() const { return dataType_; }

            std::string name() const { return name_; }

            std::string toString() const {
                return name_ + ":" + to_string(dataType_);
            }
        };

        class VariableRef : public Expr {
            std::string name_;
        public:
            VariableRef(SourceSpan sourceSpan, std::string name) : Expr(sourceSpan), name_{ name } {

            }

            NodeKind nodeKind() const override { return NodeKind::VariableRef; }

            DataType dataType() const override {
                //TODO:  after symbol table resolution, return data type from VariableDef
                return DataType::Int32;
            }

            std::string name() const { return name_; }

            std::string toString() const {
                return name_ + ":" + to_string(this->dataType());
            }
        };

        class AssignVariable : public Expr {
            std::shared_ptr<const VariableDef> variable_;  //Note:  variables are owned by LWNN::Scope.
            const std::unique_ptr<const Expr> valueExpr_;
        public:
            AssignVariable(SourceSpan sourceSpan,
                           std::shared_ptr<const VariableDef> variable,
                           std::unique_ptr<const Expr> valueExpr)
                : Expr(sourceSpan), variable_(variable), valueExpr_(move(valueExpr)) {}

            NodeKind nodeKind() const override { return NodeKind::AssignVariable; }

            DataType dataType() const override { return variable_->dataType(); }

            std::string name() const { return variable_->name(); }

            const Expr *valueExpr() const { return valueExpr_.get(); }
        };

        /** Should this really inherit from Expr?  Maybe Statement. */
        class Return : public Expr {
            const std::unique_ptr<const Expr> valueExpr_;
        public:
            Return(SourceSpan sourceSpan, std::unique_ptr<const Expr> valueExpr)
                : Expr(sourceSpan), valueExpr_(move(valueExpr)) {}

            NodeKind nodeKind() const override { return NodeKind::Return; }

            DataType dataType() const override { return valueExpr_->dataType(); }

            const Expr *valueExpr() const { return valueExpr_.get(); }
        };

        class Scope {
            const std::unordered_map<std::string, std::shared_ptr<const VariableDef>> variables_;
        public:
            Scope(std::unordered_map<std::string, std::shared_ptr<const VariableDef>> variables)
                : variables_{ move(variables) } {}

            virtual ~Scope() {}

            const VariableDef *findVariable(std::string &name) const {
                auto found = variables_.find(name);
                if (found == variables_.end()) {
                    return nullptr;
                }
                return (*found).second.get();
            }

            std::vector<const VariableDef *> variables() const {
                std::vector<const VariableDef *> vars;

                for (auto &v : variables_) {
                    vars.push_back(v.second.get());
                }

                return vars;
            }
        };

        class ScopeBuilder {
            std::unordered_map<std::string, std::shared_ptr<const VariableDef>> variables_;
        public:

            ScopeBuilder &addVariable(std::shared_ptr<const VariableDef> varDecl) {
                ASSERT_NOT_NULL(varDecl);
                variables_.emplace(varDecl->name(), varDecl);
                return *this;
            }

            std::unique_ptr<const Scope> build() {
                return std::make_unique<Scope>(move(variables_));
            }
        };

        /** Contains a series of expressions. */
        class BlockExpr : public Expr {
            const std::vector<std::unique_ptr<const Expr>> expressions_;
            std::unique_ptr<const Scope> scope_;
        public:
            virtual ~BlockExpr() {}

            /** Note:  assumes ownership of the contents of the vector arguments. */
            BlockExpr(SourceSpan sourceSpan,
                      std::unique_ptr<const Scope> scope,
                      std::vector<std::unique_ptr<const Expr>> expressions)
                : Expr(sourceSpan), expressions_{ move(expressions) }, scope_{ move(scope) } {}

            NodeKind nodeKind() const override { return NodeKind::Block; }

            /** The data type of a block expression is always the data type of the last expression in the block. */
            DataType dataType() const override {
                return expressions_.back()->dataType();
            }

            const Scope *scope() const { return scope_.get(); }

            void forEach(std::function<void(const Expr *)> func) const {
                for (auto const &expr : expressions_) {
                    func(expr.get());
                }
            }
        };

        /** Helper class which makes creating Block expression instances much easier. */
        class BlockExprBuilder {
            std::vector<std::unique_ptr<const Expr>> expressions_;
            ScopeBuilder scopeBuilder_;
            SourceSpan sourceSpan_;
        public:

            BlockExprBuilder(const SourceSpan &sourceSpan_) : sourceSpan_(sourceSpan_) {}

            virtual ~BlockExprBuilder() {
            }

            BlockExprBuilder &addVariableDef(std::shared_ptr<const VariableDef> variable) {
                ASSERT_NOT_NULL(variable);
                scopeBuilder_.addVariable(variable);
                return *this;
            }

            BlockExprBuilder &addExpression(std::unique_ptr<const Expr> newExpr) {
                ASSERT_NOT_NULL(newExpr);

                expressions_.emplace_back(move(newExpr));
                return *this;
            }

            std::unique_ptr<const BlockExpr> build() {
                return std::make_unique<const BlockExpr>(sourceSpan_, scopeBuilder_.build(), move(expressions_));
            }
        };


        /** Can be the basis of an if-then-else or ternary operator. */
        class ConditionalExpr : public Expr {
            std::unique_ptr<const Expr> condition_;
            std::unique_ptr<const Expr> truePart_;
            std::unique_ptr<const Expr> falsePart_;
        public:

            /** Note:  assumes ownership of condition, truePart and falsePart.  */
            ConditionalExpr(SourceSpan sourceSpan,
                            std::unique_ptr<const Expr> condition,
                            std::unique_ptr<const Expr> truePart,
                            std::unique_ptr<const Expr> falsePart)
                : Expr(sourceSpan),
                  condition_{ move(condition) },
                  truePart_{ move(truePart) },
                  falsePart_{ move(falsePart) } {
                ASSERT_NOT_NULL(condition_);
            }

            NodeKind nodeKind() const override {
                return NodeKind::Conditional;
            }

            DataType dataType() const override {
                if (truePart_ != nullptr) {
                    return truePart_->dataType();
                } else {
                    if (falsePart_ == nullptr) {
                        return DataType::Void;
                    } else {
                        return falsePart_->dataType();
                    }
                }
            }

            const Expr *condition() const {
                return condition_.get();
            }

            const Expr *truePart() const {
                return truePart_.get();
            }

            const Expr *falsePart() const {
                return falsePart_.get();
            }
        };

        class Function : public AstNode {
            const std::string name_;
            const DataType returnType_;
            std::unique_ptr<const Scope> parameterScope_;
            std::unique_ptr<const Expr> body_;

        public:
            Function(SourceSpan sourceSpan,
                     std::string name,
                     DataType returnType,
                     std::unique_ptr<const Scope> parameterScope,
                     std::unique_ptr<const Expr> body)
                : AstNode(sourceSpan),
                  name_{ name },
                  returnType_{ returnType },
                  parameterScope_{ move(parameterScope) },
                  body_{ move(body) } {}

            NodeKind nodeKind() const override { return NodeKind::Function; }

            std::string name() const { return name_; }

            DataType returnType() const { return returnType_; }

            const Scope *parameterScope() const { return parameterScope_.get(); };

            const Expr *body() const { return body_.get(); }
        };

        class FunctionBuilder {
            const std::string name_;
            const DataType returnType_;

            SourceSpan sourceSpan_;
            BlockExprBuilder blockBuilder_;
            ScopeBuilder parameterScopeBuilder_;
        public:
            FunctionBuilder(SourceSpan sourceSpan, std::string name, DataType returnType)
                : sourceSpan_(sourceSpan), name_{ name }, returnType_{ returnType }, blockBuilder_(sourceSpan) {}

            BlockExprBuilder &blockBuilder() {
                return blockBuilder_;
            }

            /** Assumes ownership of the variable. */
            FunctionBuilder &addParameter(std::unique_ptr<const VariableDef> variable) {
                ASSERT_NOT_NULL(variable);
                parameterScopeBuilder_.addVariable(std::move(variable));
                return *this;
            };

            std::unique_ptr<const Function> build() {
                return std::make_unique<Function>(
                    sourceSpan_,
                    name_,
                    returnType_,
                    parameterScopeBuilder_.build(),
                    blockBuilder_.build());
            }
        };

        class Module {
            const std::string name_;
            const std::vector<std::unique_ptr<const Function>> functions_;
        public:
            Module(std::string name, std::vector<std::unique_ptr<const Function>> functions)
                : name_{ name }, functions_{ move(functions) } {}

            std::string name() const { return name_; }

            void forEachFunction(std::function<void( const Function*)> func) const {
                for (const auto &f : functions_) {
                    func(f.get());
                }
            }
        };

        class ModuleBuilder {
            const std::string name_;
            std::vector<std::unique_ptr<const Function>> functions_;
        public:
            ModuleBuilder(std::string name)
                : name_{ name } {}

            ModuleBuilder &addFunction(std::unique_ptr<const Function> function) {
                functions_.emplace_back(move(function));
                return *this;
            }

            ModuleBuilder &addFunction(Function *function) {
                ASSERT_NOT_NULL(function);
                functions_.emplace_back(function);
                return *this;
            }

            std::unique_ptr<const Module> build() {
                return std::make_unique<const Module>(name_, move(functions_));
            }
        };

        /** The base class providing virtual methods with empty default implementations for all visitors.
         * Node types which do not have children will only have an overload of visit(...);  Node types that do have
         * children will have overloads for visiting(...) and visited(...) member functions. */
        class AstVisitor {
        public:

            virtual void initialize() {}

            virtual void cleanUp() {}

            /** Executes before every node is visited. */
            virtual void visitingNode(const AstNode *) { }

            /** Executes after every node is visited. */
            virtual void visitedNode(const AstNode *) {}

            virtual void visitingBlock(const BlockExpr *) {}

            virtual void visitedBlock(const BlockExpr *) {}

            virtual void visitingConditional(const ConditionalExpr *) {}

            virtual void visitedConditional(const ConditionalExpr *) {}

            virtual void visitingBinary(const BinaryExpr *) {}

            virtual void visitedBinary(const BinaryExpr *) {}

            virtual void visitLiteralInt32(const LiteralInt32Expr *) {}
            virtual void visitLiteralFloat(const LiteralFloatExpr *) {}

            virtual void visitingReturn(const Return *) {}
            virtual void visitedReturn(const Return *) {}

            virtual void visitVariableRef(const VariableRef *) {}

            virtual void visitingAssignVariable(const AssignVariable *) {}

            virtual void visitedAssignVariable(const AssignVariable *) {}

            virtual void visitingFunction(const Function *) {}
            virtual void visitedFunction(const Function *) {}

            virtual void visitingModule(const Module *) {}
            virtual void visitedModule(const Module *) {}
        };

        /** Default tree walker, suitable for most purposes */
        class AstWalker {
            AstVisitor *visitor_;

            public:
            AstWalker(AstVisitor *visitor) : visitor_{visitor} {

            }

            virtual ~AstWalker() {

            }

            void walkTree(const Module *expr) {
                visitor_->initialize();

                this->walkModule(expr);

                visitor_->cleanUp();
            }

            void walkModule(const Module *module) const {
                ASSERT_NOT_NULL(module);
                visitor_->visitingModule(module);

                module->forEachFunction([this](const Function *func) { walkFunction(func); });

                visitor_->visitedModule(module);
            }

            virtual void walk(const AstNode *node) const  {

                ASSERT_NOT_NULL(node);
                switch (node->nodeKind()) {
                    case NodeKind::LiteralInt32:
                        walkLiteralInt32(static_cast<const LiteralInt32Expr*>(node));
                        break;
                    case NodeKind::LiteralFloat:
                        walkLiteralFloat(static_cast<const LiteralFloatExpr*>(node));
                        break;
                    case NodeKind::Binary:
                        walkBinary(static_cast<const BinaryExpr*>(node));
                        break;
                    case NodeKind::Block:
                        walkBlock(static_cast<const BlockExpr*>(node));
                        break;
                    case NodeKind::Conditional:
                        walkConditional(static_cast<const ConditionalExpr*>(node));
                        break;
                    case NodeKind::VariableRef:
                        walkVariableRef(static_cast<const VariableRef*>(node));
                        break;
                    case NodeKind::AssignVariable:
                        walkAssignVariable(static_cast<const AssignVariable*>(node));
                        break;
                    case NodeKind::Return:
                        walkReturn(static_cast<const Return*>(node));
                        break;
                        //TODO:  flesh out visitor for these two
                    case NodeKind::Function:
                        walkFunction(static_cast<const Function*>(node));
                        break;
                    default:
                        throw exception::UnhandledSwitchCase();
                }
            }

            void walkBlock(const BlockExpr *blockExpr) const {
                ASSERT_NOT_NULL(blockExpr);
                visitor_->visitingNode(blockExpr);
                visitor_->visitingBlock(blockExpr);

                blockExpr->forEach([this](const Expr *childExpr) { walk(childExpr); });

                visitor_->visitedBlock(blockExpr);
                visitor_->visitedNode(blockExpr);
            }

            void walkVariableRef(const VariableRef *variableRefExpr) const {
                ASSERT_NOT_NULL(variableRefExpr);
                visitor_->visitingNode(variableRefExpr);
                visitor_->visitVariableRef(variableRefExpr);
                visitor_->visitedNode(variableRefExpr);
            }

            void walkAssignVariable(const AssignVariable *assignVariableExpr) const {

                ASSERT_NOT_NULL(assignVariableExpr);
                visitor_->visitingNode(assignVariableExpr);
                visitor_->visitingAssignVariable(assignVariableExpr);

                walk(assignVariableExpr->valueExpr());

                visitor_->visitedAssignVariable(assignVariableExpr);
                visitor_->visitedNode(assignVariableExpr);
            }

            void walkReturn(const Return *returnExpr) const {

                ASSERT_NOT_NULL(returnExpr);
                visitor_->visitingNode(returnExpr);
                visitor_->visitingReturn(returnExpr);

                walk(returnExpr->valueExpr());

                visitor_->visitedReturn(returnExpr);
                visitor_->visitedNode(returnExpr);
            }

            void walkBinary(const BinaryExpr *binaryExpr) const {
                ASSERT_NOT_NULL(binaryExpr);
                visitor_->visitingNode(binaryExpr);
                visitor_->visitingBinary(binaryExpr);

                walk(binaryExpr->lValue());
                walk(binaryExpr->rValue());

                visitor_->visitedBinary(binaryExpr);
                visitor_->visitedNode(binaryExpr);
            }

            void walkConditional(const ConditionalExpr *conditionalExpr) const {
                ASSERT_NOT_NULL(conditionalExpr);
                visitor_->visitingNode(conditionalExpr);
                visitor_->visitingConditional(conditionalExpr);

                walk(conditionalExpr->condition());
                if (conditionalExpr->truePart()) {
                    walk(conditionalExpr->truePart());
                }

                if (conditionalExpr->falsePart()) {
                    walk(conditionalExpr->falsePart());
                }

                visitor_->visitedConditional(conditionalExpr);
                visitor_->visitedNode(conditionalExpr);
            }

            void walkLiteralInt32(const LiteralInt32Expr *expr) const {
                ASSERT_NOT_NULL(expr);
                visitor_->visitingNode(expr);
                visitor_->visitLiteralInt32(expr);
                visitor_->visitedNode(expr);
            }

            void walkLiteralFloat(const LiteralFloatExpr *expr) const {
                ASSERT_NOT_NULL(expr);
                visitor_->visitingNode(expr);
                visitor_->visitLiteralFloat(expr);
                visitor_->visitedNode(expr);
            }

            void walkFunction(const Function *func) const {
                ASSERT_NOT_NULL(func);
                visitor_->visitingNode(func);
                visitor_->visitingFunction(func);

                walk(func->body());

                visitor_->visitedFunction(func);
                visitor_->visitedNode(func);
            }
        };
    } //namespace ast
} //namespace lwnn
