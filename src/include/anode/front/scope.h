
#pragma once

#include "common/exception.h"
#include "type.h"
#include "front/unique_id.h"
#include "common/string.h"
#include <string>

namespace anode { namespace front { namespace scope {

extern std::string createRandomUniqueName();

/** Indicates type of storage for this variable. */
enum class StorageKind : unsigned char {
    NotSet,
    Global,
    Local,
    Argument,
    Instance,
    TemplateParameter
};


class Symbol;

class VariableSymbol;

class FunctionSymbol;

class TypeSymbol;

class AliasSymbol;

const std::string ScopeSeparator = "::";

class SymbolTable : public gc, no_copy, no_assign {

    SymbolTable *parent_ = nullptr;
    gc_unordered_map<std::string, scope::Symbol *> symbols_;
    gc_vector<scope::Symbol *> orderedSymbols_;
    StorageKind storageKind_;
    std::string name_;

    void appendName(std::string &appendTo) {
        if (parent_) {
            parent_->appendName(appendTo);
            appendTo.append(ScopeSeparator);
        }
        appendTo.append(name());
    }

public:
    explicit SymbolTable(StorageKind storageKind, const std::string &name = "") : storageKind_(storageKind), name_{name} {}

    SymbolTable *parent() {
        return parent_;
    }

    void setParent(SymbolTable *parent) {
        ASSERT(parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = parent;
    }

    std::string name() {
        if (name_.empty()) {
            name_ = createRandomUniqueName();
        }
        return name_;
    }

    std::string fullName() {
        std::string fn;
        appendName(fn);
        return fn;
    }

    void setName(const std::string &name) {
        ASSERT(name_.empty() && "Shouldn't mutate name after it has been set.");
        name_ = name;
    }

    StorageKind storageKind() const {
        return storageKind_;
    }

    void setStorageKind(StorageKind kind) {
        storageKind_ = kind;
    }

    /** Finds the named symbol in the current scope */
    Symbol *findSymbol(const std::string &name) const;

    /** Finds the named symbol in the current scope or any parent. */
    Symbol *recursiveFindSymbol(const std::string &name) const;

    void addSymbol(Symbol *symbol);

    gc_vector<VariableSymbol *> variables();

    gc_vector<TypeSymbol *> types();

    gc_vector<FunctionSymbol *> functions() const;

    gc_vector<Symbol *> symbols() const;
};

class Symbol : public Object, no_copy, no_assign {
public:
    virtual UniqueId symbolId() = 0;
    virtual bool isFullyQualified() = 0;
    virtual void fullyQualify(SymbolTable *symbolTable) = 0;
    virtual std::string name() const = 0;
    virtual std::string toString() const = 0;
    virtual type::Type *type() const = 0;
    virtual std::string fullyQualifiedName() = 0;
    virtual StorageKind storageKind() const = 0;
    virtual void setStorageKind(StorageKind storageKind) = 0;
    virtual bool isExternal() const = 0;
    virtual Symbol *cloneForExport() = 0;
};

class SymbolBase : public Symbol {
    UniqueId symbolId_;
    bool isExternal_ = false;
    StorageKind storageKind_ = StorageKind::NotSet;
    std::string fullyQualifiedName_;
protected:
    /** "Cloning" constructor */
    explicit SymbolBase(const SymbolBase &other);

    Symbol *markExternal() {
        isExternal_ = true;
        return this;
    }

public:
    SymbolBase();

    virtual ~SymbolBase() = default;

    UniqueId symbolId() override { return symbolId_; }

    bool isFullyQualified() override { return !fullyQualifiedName_.empty(); }

    void fullyQualify(SymbolTable *symbolTable) override {
        ASSERT(fullyQualifiedName_.empty() &&
               "Attempting to do fully qualify a symbol that's already been fully qualified is probably a bug.");

        if (fullyQualifiedName_.empty())
            fullyQualifiedName_ = symbolTable->fullName() + ScopeSeparator + name();
    }

    std::string fullyQualifiedName() override {
        ASSERT(!fullyQualifiedName_.empty());
        return fullyQualifiedName_;
    }

    StorageKind storageKind() const override { return storageKind_; }

    void setStorageKind(StorageKind storageKind) override { storageKind_ = storageKind; }

    /** True when the symbol is defined in a module other than the current module.
     * Note:  this value is ignored for non-static class fields, function arguments and local variables. */
    bool isExternal() const override { return isExternal_; }
};

class VariableSymbol : public SymbolBase {
    type::Type *type_;
    std::string name_;

    /** "Cloning" constructor. */
    explicit VariableSymbol(const VariableSymbol &other) : SymbolBase(other), type_{other.type_}, name_{other.name_} {}

public:
    VariableSymbol(const std::string &name, type::Type *type) : SymbolBase(), type_{type}, name_{name} {}

    virtual type::Type *type() const override {
        ASSERT(type_ && "Type must be resolved first");
        return type_;
    }

    virtual std::string name() const override {
        return name_;
    }

    virtual std::string toString() const override {
        return name_ + ":" + (type() ? type()->name() : "<unresolved type>");
    }

    Symbol *cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return (new VariableSymbol(*this))->markExternal();
    }
};

class FunctionSymbol : public SymbolBase {
    std::string name_;
    type::FunctionType *functionType_;
    VariableSymbol *thisSymbol_ = nullptr;

    FunctionSymbol(const FunctionSymbol &other)
        : SymbolBase(other),
          name_{other.name_},
          functionType_{other.functionType_},
          thisSymbol_{other.thisSymbol_ ? static_cast<VariableSymbol *>(other.thisSymbol_->cloneForExport()) : nullptr} {}

public:
    FunctionSymbol(const std::string &name, type::FunctionType *functionType) : name_{std::move(name)}, functionType_{functionType} {}

    std::string name() const override { return name_; };

    std::string toString() const override { return name_ + ":" + functionType_->returnType()->name() + "()"; }

    type::Type *type() const override { return functionType_; }

    /** This is just a convenience so we don't have to upcast the return value of type() when we need an instance of FunctionType. */
    type::FunctionType *functionType() { return functionType_; }

    /** The symbol to be used by the "this" argument within instance methods.  */
    VariableSymbol *thisSymbol() {
        return thisSymbol_;
    }

    void setThisSymbol(VariableSymbol *thisSymbol) {
        ASSERT(storageKind() == StorageKind::Instance && "Invalid to set thisSymbol for non-instance functions.");
        thisSymbol_ = thisSymbol;
    }

    Symbol *cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return (new FunctionSymbol(*this))->markExternal();
    }
};

class TemplateSymbol : public SymbolBase {
    std::string name_;
    UniqueId astNodeId_;
    TemplateSymbol(const TemplateSymbol &other) : SymbolBase(other), name_{other.name_}, astNodeId_{other.astNodeId_} { }

public:
    TemplateSymbol(const std::string &name, UniqueId astNodeId) : name_{name}, astNodeId_(astNodeId) { }

    std::string name() const override { return name_; }
    std::string toString() const override { return string::format("%s-%d", name_.c_str(), astNodeId_); }
    type::Type *type() const override { return &type::Primitives::Void; }
    UniqueId astNodeId() { return astNodeId_; }


    Symbol *cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return (new TemplateSymbol(*this))->markExternal();
    }
};


class TypeSymbol : public SymbolBase {
    std::string name_;
    type::Type *type_;
    TypeSymbol(const TypeSymbol &other) : SymbolBase(other), name_{other.name_}, type_{other.type_} {}

public:
    TypeSymbol(type::Type *type) : name_{type->name()}, type_(type) {}
    TypeSymbol(const std::string &name, type::Type *type) : name_{name}, type_(type) {}

    std::string name() const override { return name_; }

    std::string toString() const override { return type_->name(); }

    type::Type *type() const override { return type_->actualType(); }

    Symbol *cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return (new TypeSymbol(*this))->markExternal();
    }
};




}}}
