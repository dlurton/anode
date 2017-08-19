
#pragma once

#include "common/exception.h"
#include "type.h"

#include <string>

namespace lwnn {  namespace scope {

/** Indicates type of storage for this variable. */
enum class StorageKind {
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

class VariableSymbol : public Symbol, public type::TypeResolutionListener {
    type::Type *type_ = nullptr;
    std::string name_;
    std::vector<type::TypeResolutionListener*> typeResolutionListeners_;
public:
    VariableSymbol(const std::string &name) : name_(name) { }
    VariableSymbol(const std::string &name, type::Type *type) : type_(type), name_(name) { }

    virtual ~VariableSymbol() { }

    virtual type::Type *type() const override {
        ASSERT(type_ && "Type must be resolved first");
        return type_;
    }

    virtual type::Type *maybeUnresolvedType() const {
        return type_;
    }

    void notifyTypeResolved(type::Type *type) override  {
        type_ = type;
        for(auto listener : typeResolutionListeners_) {
            listener->notifyTypeResolved(type_);
        }
    }

    void addTypeResolutionListener(type::TypeResolutionListener *symbol) {
        typeResolutionListeners_.push_back(symbol);
    }

    virtual std::string name() const override {
        return name_;
    }

    virtual std::string toString() const override {
        return name_ + ":" + (type() ? type()->name() : "<unresolved type>");
    }
};

class FunctionSymbol : public Symbol, public type::TypeResolutionListener {
    std::string name_;
    type::FunctionType *functionType_ = nullptr;
public:
    FunctionSymbol(const std::string &name) : name_(name) { }

    std::string name() const override { return name_; };
    std::string toString() const override { return name_ + ":" + functionType_->returnType()->name() + "()"; }
    type::Type *type() const override { return functionType_; }

    /** This is just a convenience so we don't have to upcast the return value of type() when we need an instance of FunctionType. */
    type::FunctionType *functionType() { return functionType_; }

    void notifyTypeResolved(type::Type *type) override  {
        functionType_ = new type::FunctionType(type);
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

class SymbolTable {
    SymbolTable *parent_ = nullptr;
    std::unordered_map<std::string, scope::Symbol*> symbols_;
    std::vector<scope::Symbol*> orderedSymbols_;
    StorageKind storageKind_;
public:
    SymbolTable(StorageKind storageKind) : storageKind_(storageKind) {}

    void setParent(SymbolTable *parent) {
        ASSERT(parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = parent;
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

    std::vector<VariableSymbol*> variables() {
        std::vector<VariableSymbol*> variables;
        for (auto symbol : orderedSymbols_) {
            auto variable = dynamic_cast<VariableSymbol*>(symbol);
            if(variable)
                variables.push_back(variable);
        }

        return variables;
    }

    std::vector<TypeSymbol*> types() {
        std::vector<TypeSymbol*> classes;
        for (auto symbol : orderedSymbols_) {
            auto variable = dynamic_cast<scope::TypeSymbol*>(symbol);
            if(variable)
                classes.push_back(variable);
        }

        return classes;
    }

    std::vector<FunctionSymbol*> functions() const {
        std::vector<FunctionSymbol*> symbols;
        for (auto symbol : orderedSymbols_) {
            auto function = dynamic_cast<FunctionSymbol*>(symbol);
            if(function != nullptr) {
                symbols.push_back(function);
            }
        }

        return symbols;
    }

    std::vector<Symbol*> symbols() const {
        std::vector<Symbol*> symbols;
        symbols.reserve(orderedSymbols_.size());
        for (auto symbol : orderedSymbols_) {
            symbols.push_back(symbol);
        }

        return symbols;
    }
};
}}
