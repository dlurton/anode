
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

class Symbol : public gc {
    bool isExternal_ = false;
    StorageKind storageKind_ = StorageKind::NotSet;
public:
    Symbol() { }
    Symbol(bool isExternal) : isExternal_{isExternal} { }
    virtual ~Symbol() { }
    virtual std::string name() const = 0;
    virtual std::string toString() const = 0;
    virtual type::Type *type() const = 0;

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
    VariableSymbol(const std::string &name, type::Type *type) : type_(type), name_(name) { }

    virtual ~VariableSymbol() { }

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
public:
    FunctionSymbol(const std::string &name, type::FunctionType *functionType) : name_{name}, functionType_{functionType} { }

    std::string name() const override { return name_; };
    std::string toString() const override { return name_ + ":" + functionType_->returnType()->name() + "()"; }
    type::Type *type() const override { return functionType_; }

    /** This is just a convenience so we don't have to upcast the return value of type() when we need an instance of FunctionType. */
    type::FunctionType *functionType() { return functionType_; }
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

class SymbolTable {
    SymbolTable *parent_ = nullptr;
    gc_unordered_map<std::string, scope::Symbol*> symbols_;
    gc_vector<scope::Symbol*> orderedSymbols_;
    StorageKind storageKind_;
    std::string name_;
public:
    SymbolTable(StorageKind storageKind) : storageKind_(storageKind) {}

    void setParent(SymbolTable *parent) {
        ASSERT(parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = parent;
    }

    std::string name() {
        if(name_.size() == 0) {
            name_ = createRandomUniqueName();
        }
        return name_;
    }

    void setName(const std::string &name) {
        ASSERT(name_.size() == 0 && "Shouldn't mutate name after it has been set.");
        name_ = name;
    }

    StorageKind storageKind() {
        return storageKind_;
    }

    void setStorageKind(StorageKind kind) {
        storageKind_ = kind;
    }

    /** Finds the named symbol in the current scope */
    Symbol* findSymbol(const std::string &name) const {
        auto found = symbols_.find(name);
        if (found == symbols_.end()) {
            return nullptr;
        }
        return found->second;
    }

    /** Finds the named symbol in the current scope or any parent. */
    Symbol* recursiveFindSymbol(const std::string &name) const {
        SymbolTable const *current = this;
        while(current) {
            Symbol* found = current->findSymbol(name);
            if(found) {
                return found;
            }
            current = current->parent_;
        }

        return nullptr;
    }

    void addSymbol(Symbol *symbol) {
        ASSERT(symbols_.find(symbol->name()) == symbols_.end()
               && "The symbol being added must not already exist in the current scope.");

        symbol->setStorageKind(storageKind_);
        symbols_.emplace(symbol->name(), symbol);
        orderedSymbols_.emplace_back(symbol);
    }

    gc_vector<VariableSymbol*> variables() {
        gc_vector<VariableSymbol*> variables;
        for (auto symbol : orderedSymbols_) {
            auto variable = dynamic_cast<VariableSymbol*>(symbol);
            if(variable)
                variables.push_back(variable);
        }

        return variables;
    }

    gc_vector<TypeSymbol*> types() {
        gc_vector<TypeSymbol*> classes;
        for (auto symbol : orderedSymbols_) {
            auto variable = dynamic_cast<scope::TypeSymbol*>(symbol);
            if(variable)
                classes.push_back(variable);
        }

        return classes;
    }

    gc_vector<FunctionSymbol*> functions() const {
        gc_vector<FunctionSymbol*> symbols;
        for (auto symbol : orderedSymbols_) {
            auto function = dynamic_cast<FunctionSymbol*>(symbol);
            if(function != nullptr) {
                symbols.push_back(function);
            }
        }

        return symbols;
    }

    gc_vector<Symbol*> symbols() const {
        gc_vector<Symbol*> symbols;
        symbols.reserve(orderedSymbols_.size());
        for (auto symbol : orderedSymbols_) {
            symbols.push_back(symbol);
        }

        return symbols;
    }
};
}}
