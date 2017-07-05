#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "Exception.hpp"

/** Rules for AST nodes:
 *      - Every node shall "own" its child nodes so that when a node is deleted, all of its children are also deleted.
 *      - Thou shalt not modify any AST node after it has been created.
 */

namespace lwnn {

    using std::make_unique;
    using std::make_shared;
    using std::shared_ptr;
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
        Module,
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
            : line(position), position(column) { }
    };

    /** Represents a range within a source defined by a starting SourceLocation and and ending SourceLocation.*/
    struct SourceSpan {
        /** The name of the input i.e. a filename, "stdin" or "REPL". */
        const std::string name;
        const SourceLocation start;
        const SourceLocation end;

        SourceSpan(std::string filename, SourceLocation start, SourceLocation end)
            : name(filename), start(start), end(end) {  }
    };

    /** Base class for all nodes */
    class Node {
    private:
        SourceSpan sourceRange_;
    public:
        Node(SourceSpan sourceRange) : sourceRange_(sourceRange) { }
        virtual ~Node() { }
        virtual NodeKind nodeKind() const = 0;
    };

    /** Base class for all expressions. */
    class Expr : public Node {
    public:
        Expr(SourceSpan sourceRange) : Node(sourceRange) { }
        virtual ~Expr() { }
        virtual DataType dataType() const { return DataType::Void; }
    };

    /** Represents an expression that is a literal 32 bit integer. */
    class LiteralInt32 : public Expr {
        int const value_;
    public:
        LiteralInt32(SourceSpan sourceRange, const int value) : Expr(sourceRange), value_(value) { }

        virtual ~LiteralInt32() { }

        NodeKind nodeKind() const override { return NodeKind::LiteralInt32; }

        DataType dataType() const override {
            return DataType::Int32;
        }

        int value() const {
            return value_;
        }
    };


    /** Represents an expression that is a literal float. */
    class LiteralFloat : public Expr {
        float const value_;
    public:
        LiteralFloat(SourceSpan sourceRange, const float value) : Expr(sourceRange), value_(value) { }

        virtual ~LiteralFloat() { }

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
    class Binary : public Expr {
        const std::unique_ptr<const Expr> lValue_;
        const OperationKind operation_;
        const std::unique_ptr<const Expr> rValue_;
    public:

        /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
        Binary(SourceSpan sourceRange, std::unique_ptr<const Expr> lValue, OperationKind operation, std::unique_ptr<const Expr> rValue)
                : Expr(sourceRange), lValue_(move(lValue)), operation_(operation), rValue_(move(rValue)) {
        }

        virtual ~Binary() { }

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
    class Variable {
        const std::string name_;
        const DataType dataType_;

    public:
        Variable(SourceSpan sourceRange, const std::string &name_, const DataType dataType) : name_(name_), dataType_(dataType) { }

        DataType dataType() const { return dataType_; }

        std::string name() const { return name_; }

        std::string toString() const {
            return name_ + ":" + to_string(dataType_);
        }
    };

    class VariableRef : public Expr {
        std::string name_;
    public:
        VariableRef(SourceSpan sourceRange, std::string name) : Expr(sourceRange), name_{name} {

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
        shared_ptr<const Variable> variable_;  //Note:  variables are owned by LWNN::Scope.
        const std::unique_ptr<const Expr> valueExpr_;
    public:
        AssignVariable(SourceSpan sourceRange,
                       std::shared_ptr<const Variable> variable,
                       std::unique_ptr<const Expr> valueExpr)
                : Expr(sourceRange), variable_(variable), valueExpr_(move(valueExpr)) { }

        NodeKind nodeKind() const override { return NodeKind::AssignVariable; }
        DataType dataType() const override { return variable_->dataType(); }

        std::string name() const { return variable_->name(); }
        const Expr* valueExpr() const { return valueExpr_.get(); }
    };

    /** Should this really inherit from Expr?  Maybe Statement. */
    class Return : public Expr {
        const std::unique_ptr<const Expr> valueExpr_;
    public:
        Return(SourceSpan sourceRange, std::unique_ptr<const Expr> valueExpr)
                : Expr(sourceRange), valueExpr_(move(valueExpr)) { }

        NodeKind nodeKind() const override { return NodeKind::Return; }
        DataType dataType() const override { return valueExpr_->dataType(); }

        const Expr* valueExpr() const { return valueExpr_.get(); }
    };

    class Scope {
        const std::unordered_map<std::string, shared_ptr<const Variable>> variables_;
    public:
        Scope(std::unordered_map<std::string, shared_ptr<const Variable>> variables)
                : variables_{move(variables)} { }

        virtual ~Scope() {}

        const Variable *findVariable(std::string &name) const {
            auto found = variables_.find(name);
            if(found == variables_.end()) {
                return nullptr;
            }
            return (*found).second.get();
        }

        std::vector<const Variable*> variables() const {
            std::vector<const Variable*> vars;

            for(auto &v : variables_) {
                vars.push_back(v.second.get());
            }

            return vars;
        }
    };

    class ScopeBuilder {
        std::unordered_map<std::string, shared_ptr<const Variable>> variables_;
    public:

        ScopeBuilder &addVariable(shared_ptr<const Variable> varDecl) {
            variables_.emplace(varDecl->name(), varDecl);
            return *this;
        }

        std::unique_ptr<const Scope> build() {
            return std::make_unique<Scope>(move(variables_));
        }
    };

    /** Contains a series of expressions. */
    class Block : public Expr {
        const std::vector<std::unique_ptr<const Expr>> expressions_;
        std::unique_ptr<const Scope> scope_;
    public:
        virtual ~Block() {}

        /** Note:  assumes ownership of the contents of the vector arguments. */
        Block(SourceSpan sourceRange,
              std::unique_ptr<const Scope> scope,
              std::vector<std::unique_ptr<const Expr>> expressions)
                : Expr(sourceRange), expressions_{move(expressions)}, scope_{move(scope)} { }

        NodeKind nodeKind() const override { return NodeKind::Block; }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const override {
            return expressions_.back()->dataType();
        }

        const Scope *scope() const { return scope_.get(); }

        void forEach(std::function<void(const Expr*)> func) const {
            for(auto const &expr : expressions_) {
                func(expr.get());
            }
        }
    };
//
//    /** Helper class which makes creating Block expression instances much easier. */
//    class BlockBuilder {
//        std::vector<std::unique_ptr<const Expr>> expressions_;
//        ScopeBuilder scopeBuilder_;
//    public:
//        virtual ~BlockBuilder() {
//        }
//
//        BlockBuilder &addVariable(shared_ptr<const Variable> variable) {
//            scopeBuilder_.addVariable(variable);
//            return *this;
//        }
//
//        BlockBuilder &addExpression(std::unique_ptr<const Expr> newExpr) {
//            expressions_.emplace_back(move(newExpr));
//            return *this;
//        }
//
//        std::unique_ptr<const Block> build(SourceSpan sourceSpan) {
//            return std::make_unique<const Block>(sourceSpan, scopeBuilder_.build(), move(expressions_));
//        }
//    };


    /** Can be the basis of an if-then-else or ternary operator. */
    class Conditional : public Expr {
        std::unique_ptr<const Expr> condition_;
        std::unique_ptr<const Expr> truePart_;
        std::unique_ptr<const Expr> falsePart_;
    public:

        /** Note:  assumes ownership of condition, truePart and falsePart.  */
        Conditional(SourceSpan sourceRange,
                    std::unique_ptr<const Expr> condition,
                    std::unique_ptr<const Expr> truePart,
                    std::unique_ptr<const Expr> falsePart)
                : Expr(sourceRange),
                  condition_{move(condition)},
                  truePart_{move(truePart)},
                  falsePart_{move(falsePart)}
        {
            ARG_NOT_NULL(condition_);
        }

        NodeKind nodeKind() const override {
            return NodeKind::Conditional;
        }

        DataType dataType() const override {
            if(truePart_ != nullptr) {
                return truePart_->dataType();
            } else {
                if(falsePart_ == nullptr) {
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

    class Function : public Node {
        const std::string name_;
        const DataType returnType_;
        std::unique_ptr<const Scope> parameterScope_;
        std::unique_ptr<const Expr> body_;

    public:
        Function(SourceSpan sourceRange,
                 std::string name,
                 DataType returnType,
                 std::unique_ptr<const Scope> parameterScope,
                 std::unique_ptr<const Expr> body)
                : Node(sourceRange),
                  name_{name},
                  returnType_{returnType},
                  parameterScope_{move(parameterScope)},
                  body_{move(body)} { }

        NodeKind nodeKind() const override { return NodeKind::Function; }

        std::string name() const { return name_; }

        DataType returnType() const { return returnType_; }

        const Scope *parameterScope() const { return parameterScope_.get(); };

        const Expr *body() const { return body_.get(); }

    };

//    class FunctionBuilder {
//        const std::string name_;
//        const DataType returnType_;
//
//        BlockBuilder blockBuilder_;
//        ScopeBuilder parameterScopeBuilder_;
//    public:
//        FunctionBuilder(SourceSpan sourceRange, std::string name, DataType returnType)
//            : name_{name}, returnType_{returnType} { }
//
//        BlockBuilder &blockBuilder() {
//            return blockBuilder_;
//        }
//
//        /** Assumes ownership of the variable. */
//        FunctionBuilder &addParameter(std::unique_ptr<const Variable> variable) {
//            parameterScopeBuilder_.addVariable(move(variable));
//            return *this;
//        };
//
//        std::unique_ptr<const Function> build(SourceSpan sourceSpan) {
//            return std::make_unique<Function>(name_,
//                                         returnType_,
//                                         parameterScopeBuilder_.build(),
//                                         blockBuilder_.build(sourceSpan));
//        }
//    };

    class Module : public Node {
        const std::string name_;
        const std::vector<std::unique_ptr<const Function>> functions_;
    public:
        Module(SourceSpan range, std::string name, std::vector<std::unique_ptr<const Function>> functions)
             : Node(range), name_{name}, functions_{move(functions)}  { }

        NodeKind nodeKind() const override { return NodeKind::Module; }

        std::string name() const { return name_; }

        void forEachFunction(std::function<void(const Function *)> func) const {
            for(const auto &f : functions_) {
                func(f.get());
            }
        }
    };

//    class ModuleBuilder {
//        const std::string name_;
//        std::vector<std::unique_ptr<const Function>> functions_;
//    public:
//        ModuleBuilder(std::string name)
//            : name_{name}
//        { }
//
//        ModuleBuilder &addFunction(std::unique_ptr<const Function> function) {
//            functions_.emplace_back(move(function));
//            return *this;
//        }
//
//        ModuleBuilder &addFunction(Function *function) {
//            functions_.emplace_back(function);
//            return *this;
//        }
//
//        std::unique_ptr<const Module> build() {
//            return std::make_unique<const Module>(name_, move(functions_));
//        }
//    };
}
