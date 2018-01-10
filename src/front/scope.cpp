
#include "front/unique_id.h"

#include <uuid/uuid.h>
#include <string>
#include <front/scope.h>
#include <atomic>

namespace anode { namespace front { namespace scope {

//the cloning constructor
SymbolBase::SymbolBase(const SymbolBase &other)
    :   symbolId_{GetNextUniqueId()},  //Clones however must be allocated a different SymbolId.
        isExternal_{other.isExternal_},
        storageKind_{other.storageKind_},
        fullyQualifiedName_{other.fullyQualifiedName_},
        mySymbolTable_{other.mySymbolTable_}
{

}

SymbolBase::SymbolBase()
    : symbolId_{GetNextUniqueId()}
{

}

Symbol *SymbolTable::findSymbolInCurrentScope(const std::string &name) const {
    auto found = symbols_.find(name);
    if (found == symbols_.end()) {
        return nullptr;
    }
    return &found->second.get();
}

Symbol *SymbolTable::findSymbolInCurrentScopeOrParents(const std::string &name) const {
    SymbolTable const *current = this;
    while(current) {
        Symbol* found = current->findSymbolInCurrentScope(name);
        if(found) {
            return found;
        }
        current = current->parent_;
    }

    return nullptr;
}

void SymbolTable::addSymbol(Symbol &symbol) {
    ASSERT(storageKind_ != StorageKind::NotSet);
    ASSERT(!symbol.name().empty());
#ifdef ANODE_DEBUG
    if(findSymbolInCurrentScope(symbol.name())) {
        throw exception::DebugAssertionFailedException(string::format("Symbol '%s' already exists in this SymbolTable", symbol.name().c_str()));
    }
#endif

    ASSERT(!findSymbolInCurrentScope(symbol.name()));

    if(!symbol.isFullyQualified()) {
        symbol.fullyQualify(this);
    }
    symbol.setStorageKind(storageKind_);
    symbols_.emplace(symbol.name(), symbol);
    orderedSymbols_.emplace_back(symbol);
}

gc_ref_vector<VariableSymbol> SymbolTable::variables() {
    gc_ref_vector<VariableSymbol> variables;
    for (auto symbol : orderedSymbols_) {
        auto variable = dynamic_cast<VariableSymbol*>(&symbol.get());
        if(variable)
            variables.emplace_back(*variable);
    }

    return variables;
}

gc_ref_vector<TypeSymbol> SymbolTable::types() {
    gc_ref_vector<TypeSymbol> classes;
    for (auto symbol : orderedSymbols_) {
        auto type = dynamic_cast<scope::TypeSymbol*>(&symbol.get());
        if(type)
            classes.emplace_back(*type);
    }

    return classes;
}

gc_ref_vector<FunctionSymbol> SymbolTable::functions() const {
    gc_ref_vector<FunctionSymbol> symbols;
    for (auto symbol : orderedSymbols_) {
        auto function = dynamic_cast<FunctionSymbol*>(&symbol.get());
        if(function != nullptr) {
            symbols.emplace_back(*function);
        }
    }

    return symbols;
}

gc_ref_vector<Symbol> SymbolTable::symbols() const {
    return orderedSymbols_;
}

}}}