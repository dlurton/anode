
#pragma once

#include "common/exception.h"
#include "type.h"
#include "front/unique_id.h"
#include "common/string.h"
#include <string>

namespace anode { namespace front { namespace scope {

/** Indicates type of storage for variables defined within for a symbol table. */
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




class SymbolTable : public gc {

public:
    virtual SymbolTable *parent() const = 0;
    virtual ~SymbolTable() { }

    virtual void setParent(SymbolTable &parent) = 0;

    virtual std::string name() const = 0;

    virtual std::string fullName() = 0;

    virtual StorageKind storageKind() const = 0;
    virtual void setStorageKind(StorageKind kind) = 0;

    /** Finds the named symbol in the current scope */
    virtual Symbol *findSymbolInCurrentScope(const std::string &name) const = 0;

    /** Finds the named symbol in the current scope or any parent. */
    virtual Symbol *findSymbolInCurrentScopeOrParents(const std::string &name) const = 0;

    virtual void addSymbol(Symbol &symbol) = 0;

    virtual gc_ref_vector<VariableSymbol> variables() = 0;

    virtual gc_ref_vector<TypeSymbol> types() = 0;

    virtual gc_ref_vector<FunctionSymbol> functions() const = 0;

    virtual gc_ref_vector<Symbol> symbols() const = 0;
};

class ScopeSymbolTable : public SymbolTable {

    gc_ref_unordered_map<std::string, scope::Symbol> symbols_;
    gc_ref_vector<scope::Symbol> orderedSymbols_;
    StorageKind storageKind_;
    std::string name_;
    SymbolTable *parent_ = nullptr;

    void appendName(std::string &appendTo) {
        if (auto parentScope = dynamic_cast<ScopeSymbolTable*>(parent_)) {
            parentScope->appendName(appendTo);
            appendTo.append(ScopeSeparator);
        }
        appendTo.append(name());
    }

public:
    NO_COPY_NO_ASSIGN(ScopeSymbolTable)
    explicit ScopeSymbolTable(StorageKind storageKind, const std::string &name)
        : SymbolTable(), storageKind_(storageKind), name_{name} {
        ASSERT(name.length() > 0)
    }
    
    explicit ScopeSymbolTable(StorageKind storageKind, const std::string &name, SymbolTable *parent)
        : SymbolTable(), storageKind_(storageKind), name_{name}, parent_{parent} {
        ASSERT(name.length() > 0)
    }

    SymbolTable *parent() const override {
        return parent_;
    }

    void setParent(SymbolTable &parent) override {
        ASSERT(&parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = &parent;
    }

    std::string name() const override {
        return name_;
    }

    std::string fullName() override{
        std::string fn;
        appendName(fn);
        return fn;
    }

    StorageKind storageKind() const override {
        return storageKind_;
    }

    void setStorageKind(StorageKind kind) override {
        storageKind_ = kind;
    }

    /** Finds the named symbol in the current scope */
    Symbol *findSymbolInCurrentScope(const std::string &name) const override;

    /** Finds the named symbol in the current scope or any parent. */
    Symbol *findSymbolInCurrentScopeOrParents(const std::string &name) const override;

    void addSymbol(Symbol &symbol) override;

    gc_ref_vector<VariableSymbol> variables() override;

    gc_ref_vector<TypeSymbol> types() override;

    gc_ref_vector<FunctionSymbol> functions() const override;

    gc_ref_vector<Symbol> symbols() const override;
};

class DelegatingSymbolTable : public SymbolTable {

    SymbolTable *targetSymbolTable_;
    SymbolTable *parent_;
public:

    DelegatingSymbolTable(SymbolTable *targetSymbolTable)
            : targetSymbolTable_{targetSymbolTable} {
        
    }


    SymbolTable *target() const {
        return targetSymbolTable_;
    }

    SymbolTable *parent() const override {
        return parent_;
    }

    void setParent(SymbolTable &parent) override {
        ASSERT(&parent != this && "Hello? Are you trying to cause an infinite loop?");
        parent_ = &parent;
    }

    std::string name() const override {
        return targetSymbolTable_->name();
    }

    std::string fullName() override{
        return targetSymbolTable_->fullName();
    }

    StorageKind storageKind() const override {
        return targetSymbolTable_->storageKind();
    }

    void setStorageKind(StorageKind ) override {
        ASSERT_FAIL("Can't mutate DelegatingSymbolTable instances (probably should refactor so DelegatingSymbolTable doesn't need to implement mutators.")
        //targetSymbolTable_->setStorageKind(kind);
    }

    void addSymbol(Symbol &) override {
        ASSERT_FAIL("Can't mutate DelegatingSymbolTable instances (probably should refactor so DelegatingSymbolTable doesn't need to implement mutators.")
    }

    /** Finds the named symbol in the current scope */
    Symbol *findSymbolInCurrentScope(const std::string &name) const override {
        return targetSymbolTable_->findSymbolInCurrentScope(name);
    }

    /**
     * Finds the named symbol in the current scope or any parent or parents of the {@ ref DelegatingSymolTable} and
     * *not* the target symbol table.  This is in fact the whole point of this class.  We are essentially creating a s
     * sopce with the same symbols as another scope, but with different parents.
     */
    Symbol *findSymbolInCurrentScopeOrParents(const std::string &name) const override {
        Symbol *found = targetSymbolTable_->findSymbolInCurrentScope(name);
        if(found != nullptr) {
            return found;
        } else {
            return parent_->findSymbolInCurrentScopeOrParents(name);
        }
    }

    gc_ref_vector<VariableSymbol> variables() override {
        return targetSymbolTable_->variables();
    }

    gc_ref_vector<TypeSymbol> types() override {
        return targetSymbolTable_->types();
    }

    gc_ref_vector<FunctionSymbol> functions() const override {
        return targetSymbolTable_->functions();
    }

    gc_ref_vector<Symbol> symbols() const override {
        return targetSymbolTable_->symbols();
    }
};

class Symbol : public Object {
public:
    NO_COPY_NO_ASSIGN(Symbol)
    Symbol() { }
    virtual UniqueId symbolId() = 0;
    virtual bool isFullyQualified() = 0;
    virtual void fullyQualify(SymbolTable *symbolTable) = 0;
    virtual std::string name() const = 0;
    virtual std::string toString() const = 0;
    virtual type::Type &type() const = 0;
    virtual std::string fullyQualifiedName() = 0;
    virtual StorageKind storageKind() const = 0;
    virtual void setStorageKind(StorageKind storageKind) = 0;
    virtual bool isExternal() const = 0;
    virtual Symbol &cloneForExport() = 0;
};

class SymbolBase : public Symbol {
    UniqueId symbolId_;
    bool isExternal_ = false;
    StorageKind storageKind_ = StorageKind::NotSet;
    std::string fullyQualifiedName_;
    SymbolTable *mySymbolTable_ = nullptr;
protected:
    SymbolBase();
    /** "Cloning" constructor */
    explicit SymbolBase(const SymbolBase &other);

    Symbol *markExternal() {
        isExternal_ = true;
        return this;
    }

public:

    NO_ASSIGN(SymbolBase)
    virtual ~SymbolBase() = default;

    UniqueId symbolId() override { return symbolId_; }

    bool isFullyQualified() override { return !fullyQualifiedName_.empty(); }

    SymbolTable &symbolTable() {
        ASSERT(mySymbolTable_)
        return *mySymbolTable_;
    }

    void fullyQualify(SymbolTable *mySymbolTable) override {
        ASSERT(!mySymbolTable_ && "I don't think we want to change the symbol table of a symbol.")
        mySymbolTable_ = mySymbolTable;

        if (fullyQualifiedName_.empty())
            fullyQualifiedName_ = mySymbolTable_->fullName() + ScopeSeparator + name();
    }

    /**
     * For symbols that reside directly in namespaces, returns a single SymbolTable for the symbol's namespace and one for
     * each of its parents.
     */
    gc_ref_vector<SymbolTable> getNamespacesAndParents() {
        SymbolTable *current = mySymbolTable_;
        gc_ref_vector<SymbolTable> results;
        while(current != nullptr) {
            results.emplace_back(*current);
            current = current->parent();
        }
        std::reverse(results.begin(), results.end());
        return results;
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
    type::Type &type_;
    std::string name_;

    /** "Cloning" constructor. */
    explicit VariableSymbol(const VariableSymbol &other) : SymbolBase(other), type_{other.type_}, name_{other.name_} {}

public:
    VariableSymbol(const std::string &name, type::Type &type) : SymbolBase(), type_{type}, name_{name} {}

    virtual type::Type &type() const override {
        return type_;
    }

    virtual std::string name() const override {
        return name_;
    }

    virtual std::string toString() const override {
        return name_ + ":" + type().nameForDisplay();
    }

    Symbol &cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return *(new VariableSymbol(*this))->markExternal();
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
          thisSymbol_{other.thisSymbol_ ? static_cast<VariableSymbol *>(&other.thisSymbol_->cloneForExport()) : nullptr} {}

public:
    FunctionSymbol(const std::string &name, type::FunctionType *functionType) : name_{std::move(name)}, functionType_{functionType} {}

    std::string name() const override { return name_; };

    std::string toString() const override { return name_ + ":" + functionType_->returnType()->nameForDisplay() + "()"; }

    type::Type &type() const override { return *functionType_; }

    /** This is just a convenience so we don't have to upcast the return value of type() when we need an instance of FunctionType. */
    type::FunctionType *functionType() { return functionType_; }

    /** The symbol to be used by the "this" argument within instance methods.  */
    VariableSymbol *thisSymbol() {
        ASSERT(storageKind() == StorageKind::Instance);
        ASSERT(thisSymbol_);
        return thisSymbol_;
    }

    void setThisSymbol(VariableSymbol *thisSymbol) {
        ASSERT(storageKind() == StorageKind::Instance && "Invalid to set thisSymbol for non-instance functions.");
        thisSymbol_ = thisSymbol;
    }

    Symbol &cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return *(new FunctionSymbol(*this))->markExternal();
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
    type::Type &type() const override { return type::ScalarType::Void; }
    UniqueId astNodeId() { return astNodeId_; }


    Symbol &cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return *(new TemplateSymbol(*this))->markExternal();
    }
};


class TypeSymbol : public SymbolBase {
    std::string name_;
    type::Type &type_;
    TypeSymbol(const TypeSymbol &other) : SymbolBase(other), name_{other.name_}, type_{other.type_} {}

public:
    TypeSymbol(type::Type &type) : name_{type.name()}, type_(type) {}
    TypeSymbol(const std::string &name, type::Type &type) : name_{name}, type_(type) {}

    std::string name() const override { return name_; }

    std::string toString() const override { return type_.nameForDisplay(); }

    type::Type &type() const override { return type_; }

    Symbol &cloneForExport() override {
        ASSERT(storageKind() == StorageKind::Global);
        return *(new TypeSymbol(*this))->markExternal();
    }
};

class NamespaceSymbol : public SymbolBase {
    SymbolTable &symbolTable_;
public:
    NO_COPY_NO_ASSIGN(NamespaceSymbol)

    explicit NamespaceSymbol(SymbolTable &symbolTable) : symbolTable_{symbolTable} { }

    std::string name() const override { return symbolTable_.name(); }

    std::string toString() const override { return "NS: " + symbolTable_.name(); }

    type::Type &type() const override { return type::ScalarType::Void; }

    SymbolTable &symbolTable() { return symbolTable_; }

    Symbol &cloneForExport() override {
        //it's quite possible that we will need to clone sybmolTable_ here...
        return *new NamespaceSymbol(symbolTable_);
    }
};

struct ScopeInfo {
    const std::string scopeName;
    const StorageKind storageKind;
    ScopeInfo(std::string scopeName, StorageKind storageKind) : scopeName{scopeName}, storageKind{storageKind} { }
};

class ScopedNode {
public:
    virtual ScopeInfo scopeInfo() = 0;
};

class ScopeManager : gc {
    NO_COPY_NO_ASSIGN(ScopeManager);
    
    gc_unordered_map<ScopedNode*, SymbolTable*> allScopes_;
    gc_deque<SymbolTable*> scopeStack_;
    
public:
    SymbolTable *enterScope(ScopedNode *forNode) {
        SymbolTable *scope = allScopes_[forNode];
        if(scope == nullptr) {
            ScopeInfo s_info = forNode->scopeInfo();
            scope = new ScopeSymbolTable(s_info.storageKind, s_info.scopeName);
    
            allScopes_[forNode] = scope;
        }
        scopeStack_.emplace_back(scope);
        return scope;
    }
    
    SymbolTable *currentScope() {
        return scopeStack_.back();
    }
    
    void exitScope() {
        scopeStack_.pop_back();
    }
    
};

}}}
