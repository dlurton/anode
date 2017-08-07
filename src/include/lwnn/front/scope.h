
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
            virtual ~Symbol() { }
            virtual std::string name() const = 0;
            virtual std::string toString() const = 0;
            virtual type::Type *type() const = 0;
        };

        class DelayedTypeResolutionSymbol : public scope::Symbol {
            type::Type *type_ = nullptr;
        public:
            virtual ~DelayedTypeResolutionSymbol() { }
            DelayedTypeResolutionSymbol() {}
            //This exists in to handle the scenario where an inheritor needs to support delayed type resolution only sometimes
            DelayedTypeResolutionSymbol(type::Type *type) : type_(type) { }

            virtual type::Type *type() const override {
                ASSERT(type_ && "Type must be resolved first");
                return type_;
            }

            void setType(type::Type *type) {
                type_ = type;
            }
        };

        class GlobalVariableSymbol : public scope::DelayedTypeResolutionSymbol {
        private:
            std::string name_;
        public:
            GlobalVariableSymbol(const std::string &name) : DelayedTypeResolutionSymbol(), name_(name) { }
            GlobalVariableSymbol(const std::string &name, type::Type *type) : DelayedTypeResolutionSymbol(type), name_(name) { }
            virtual ~GlobalVariableSymbol() { }

            virtual std::string name() const override {
                return name_;
            }

            virtual std::string toString() const override {
                return name_ + ":" + (type() ? type()->name() : "<unresolved type>");
            }

        };

        class ClassSymbol : public Symbol {
            type::ClassType *type_;
        public:
            ClassSymbol(type::ClassType *type) : type_(type) {

            }
            std::string name() const override { return type_->name(); }
            std::string toString() const override { return type_->name(); }
            type::Type *type() const override { return type_; }

            /** Honestly, just a convenience method so we don't have to upcast from Type* like we would have to if we only had the type()
             * member function. */
            type::ClassType* classType() const { return type_; }
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
                return (*found).second;
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
