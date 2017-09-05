

#include <uuid/uuid.h>
#include <string>
#include <front/scope.h>


namespace anode { namespace scope {

std::string createRandomUniqueName() {
    uuid_t uuid;
    uuid_generate(uuid);
    // unparse (to string)
    char uuid_str[37];      // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
    uuid_unparse_lower(uuid, uuid_str);

    std::string str{uuid_str};
    str.reserve(32);

    for(int i = 0; uuid_str[i] != 0; ++i) {
        if(uuid_str[i] != '-') {
            str.push_back(uuid_str[i]);
        }
    }
    return str;
}

anode::scope::Symbol *anode::scope::SymbolTable::findSymbol(const std::string &name) const {
    auto found = symbols_.find(name);
    if (found == symbols_.end()) {
        return nullptr;
    }
    return found->second;
}

Symbol *SymbolTable::recursiveFindSymbol(const std::string &name) const {
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

void SymbolTable::addSymbol(Symbol *symbol) {
    if(!symbol->isFullyQualified()) {
        symbol->fullyQualify(this);
    }
    symbol->setStorageKind(storageKind_);
    symbols_.emplace(symbol->name(), symbol);
    orderedSymbols_.emplace_back(symbol);
}

gc_vector<VariableSymbol *> SymbolTable::variables() {
    gc_vector<VariableSymbol*> variables;
    for (auto symbol : orderedSymbols_) {
        auto variable = dynamic_cast<VariableSymbol*>(symbol);
        if(variable)
            variables.push_back(variable);
    }

    return variables;
}

gc_vector<TypeSymbol *> SymbolTable::types() {
    gc_vector<TypeSymbol*> classes;
    for (auto symbol : orderedSymbols_) {
        auto variable = dynamic_cast<scope::TypeSymbol*>(symbol);
        if(variable)
            classes.push_back(variable);
    }

    return classes;
}

gc_vector<FunctionSymbol *> SymbolTable::functions() const {
    gc_vector<FunctionSymbol*> symbols;
    for (auto symbol : orderedSymbols_) {
        auto function = dynamic_cast<FunctionSymbol*>(symbol);
        if(function != nullptr) {
            symbols.push_back(function);
        }
    }

    return symbols;
}

gc_vector<Symbol *> SymbolTable::symbols() const {
    gc_vector<Symbol*> symbols;
    symbols.reserve(orderedSymbols_.size());
    for (auto symbol : orderedSymbols_) {
        symbols.push_back(symbol);
    }

    return symbols;
}

}}