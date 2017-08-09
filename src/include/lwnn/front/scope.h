
#pragma once

#include "common/exception.h"
#include "type.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace lwnn {
    namespace scope {

        class Symbol : public gc {
        public:
            Symbol() {}
            virtual ~Symbol() { }
            virtual std::string name() const = 0;
            virtual std::string toString() const = 0;
            virtual type::Type *type() const = 0;
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
        public:

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

            std::vector<Symbol*> symbols() const {
                std::vector<Symbol*> symbols;
                symbols.reserve(orderedSymbols_.size());
                for (auto &symbol : orderedSymbols_) {
                    symbols.push_back(symbol);
                }

                return symbols;
            }
        };
    }
}
