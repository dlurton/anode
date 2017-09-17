
#pragma once

#include "common/exception.h"
#include "type.h"

#include <string>

namespace anode {  namespace scope {

extern std::string createRandomUniqueName();

/** Indicates type of storage for this variable. */
enum class StorageKind : unsigned char {
    NotSet,
    Global,
    Local,
    Argument,
    Instance
};

class Symbol;
class VariableSymbol;
class FunctionSymbol;
class TypeSymbol;
class AliasSymbol;

const std::string ScopeSeparator = "::";

class SymbolTable {
    SymbolTable *parent_ = nullptr;
    gc_unordered_map<std::string, scope::Symbol*> symbols_;
    gc_vector<scope::Symbol*> orderedSymbols_;
    StorageKind storageKind_;
    std::string name_;


    void appendName(std::string &appendTo) {
        if(parent_) {
            parent_->appendName(appendTo);
            appendTo.append(ScopeSeparator);
        }
        appendTo.append(name());
    }
public:
    explicit SymbolTable(StorageKind storageKind) : storageKind_(storageKind) {}

    void setParent(SymbolTable *parent) {
        ASSERT(parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = parent;
    }

    std::string name() {
        if(name_.empty()) {
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

    StorageKind storageKind() {
        return storageKind_;
    }

    void setStorageKind(StorageKind kind) {
        storageKind_ = kind;
    }

    /** Finds the named symbol in the current scope */
    Symbol* findSymbol(const std::string &name) const;

    /** Finds the named symbol in the current scope or any parent. */
    Symbol* recursiveFindSymbol(const std::string &name) const;

    void addSymbol(Symbol *symbol);

    gc_vector<VariableSymbol*> variables();

    gc_vector<TypeSymbol*> types();

    gc_vector<FunctionSymbol*> functions() const;

    gc_vector<Symbol*> symbols() const;
};

class Symbol : public gc, no_copy, no_assign {
    bool isExternal_ = false;
    StorageKind storageKind_ = StorageKind::NotSet;
    std::string fullyQualifiedName_;
public:
    Symbol() = default;

    explicit Symbol(bool isExternal) : isExternal_{isExternal} { }
    virtual ~Symbol() = default;

    bool isFullyQualified() { return !fullyQualifiedName_.empty(); }
    void fullyQualify(SymbolTable *symbolTable) {
        ASSERT(fullyQualifiedName_.empty() &&
               "Attempting to do fully qualify a symbol that's already been fully qualified is probably a bug.");

        if(fullyQualifiedName_.empty())
            fullyQualifiedName_ = symbolTable->fullName() + ScopeSeparator + name();
    }

    virtual std::string name() const = 0;
    virtual std::string toString() const = 0;
    virtual type::Type *type() const = 0;

    std::string fullyQualifiedName() {
        ASSERT(!fullyQualifiedName_.empty());
        return fullyQualifiedName_;
    }

    StorageKind storageKind() const { return storageKind_; }
    void setStorageKind(StorageKind storageKind) { storageKind_ = storageKind; }

    /** True when the symbol is defined in a module other than the current module.
     * Note:  this value is ignored for non-static class fields, function arguments and local variables. */
    bool isExternal() const { return isExternal_; }
    void setIsExternal(bool isExternal) { isExternal_ = isExternal; }
};

class VariableSymbol : public Symbol {
    type::Type *type_;
    std::string name_;
public:
    VariableSymbol(const std::string &name, type::Type *type) : type_{type}, name_{name} { }

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
};


class FunctionSymbol : public Symbol {
    std::string name_;
    type::FunctionType *functionType_;
    VariableSymbol *thisSymbol_ = nullptr;
public:
    FunctionSymbol(const std::string &name, type::FunctionType *functionType) : name_{std::move(name)}, functionType_{functionType} { }

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
};

class TypeSymbol : public Symbol {
    type::Type *type_;
public:
    TypeSymbol(type::ClassType *type) : type_(type) {

    }
    std::string name() const override { return type_->name(); }
    std::string toString() const override { return type_->name(); }
    type::Type *type() const override { return type_; }
};
}}
