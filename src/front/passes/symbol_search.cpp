

#include "symbol_search.h"
namespace anode { namespace front {

/**
 * Searches for a (optionally) qualified symbol from the specified {@parameter startingScope}.
 * If {@parameter id} has one identifier in it, the effect is the same as calling SymbolTable::recurisveFindSymbol(...) directly.
 * If {@parameter id} has more than one identifier in it, searches for the first symbol up through the scope parentage chain.  If a namespace
 * ({@ref anode::front::scope::NamespaceSymbol}) is found, searches that namespace for the second symbol.  If another namespace is found,
 * searches for the third symbol and so on until either all of the symbols in {@parameter id} have been found or a non-existing symbol is
 * encountered, or when a symbol other than the last symbol resolves to something other than a namespace.
 *
 * @param startingScope
 * @param id
 * @param errorStream
 * @return The resolved symbol, or <i>nullptr</i> if none is found.
 */
scope::Symbol *findQualifiedSymbol(scope::SymbolTable &startingScope, const ast::MultiPartIdentifier &id, error::ErrorStream &errorStream) {
    //Identifier has only one element, just do a simple search up the parentage chain for that symbol.
    if (id.size() == 1) {
        scope::Symbol *foundSymbol = startingScope.findSymbolInCurrentScopeOrParents(id.front().text());
        if (foundSymbol == nullptr) {
            errorStream.error(
                error::ErrorKind::SymbolNotDefined,
                id.front().span(),
                "identifier '%s' does not exist or is not accessible from the current scope.",
                id.front().text().c_str());
        }
        return foundSymbol;
    } else {
        //The identifier has more than one element, so some complexity is involved.

        //Use entire scope parentage chain to search for the first identifier.
        scope::Symbol *maybeNamespace = startingScope.findSymbolInCurrentScopeOrParents(id.front().text());
        if (maybeNamespace == nullptr) {
            errorStream.error(
                error::ErrorKind::NamespaceDoesNotExist,
                id.front().span(),
                "namespace '%s' does not exist or is not accessible from the current scope.",
                id.front().text().c_str());
            return nullptr;
        }

        //Make sure it's a namespace.
        scope::SymbolTable *currentNamespace = nullptr;
        if (scope::NamespaceSymbol *nss = dynamic_cast<scope::NamespaceSymbol *>(maybeNamespace)) {
            currentNamespace = &nss->symbolTable();
        } else {
            errorStream.error(
                error::ErrorKind::IdentifierIsNotNamespace,
                id.front().span(),
                "identifier '%s' is not a namespace.",
                id.front().text().c_str());
            return nullptr;
        }

        std::string scopePath = id.front().text();
        //For each identifier between the first and last elements of the qualified identifier, resolve each scope without using the
        //the scope parentage chain, thereby descending through scopes.
        ast::MultiPartIdentifier::middle_vector middleParts = id.middle();
        for (const ast::Identifier &part : middleParts) {
            scope::Symbol *maybeNamespace = currentNamespace->findSymbolInCurrentScope(part.text());
            if (maybeNamespace == nullptr) {
                errorStream.error(
                    error::ErrorKind::ChildNamespaceDoesNotExist,
                    part.span(),
                    "namespace '%s' does not exist within namespace '%s'.",
                    part.text().c_str(),
                    scopePath.c_str());
                return nullptr;
            }
            //Make sure what we got was a namespace.
            if (scope::NamespaceSymbol *nss = dynamic_cast<scope::NamespaceSymbol *>(maybeNamespace)) {
                currentNamespace = &nss->symbolTable();
            } else {
                errorStream.error(
                    error::ErrorKind::MemberOfNamespaceIsNotNamespace,
                    part.span(),
                    "identifier '%s' of namespace '%s' is not a child namespace.",
                    part.text().c_str(),
                    scopePath.c_str());
                return nullptr;
            }
            scopePath += scope::ScopeSeparator;
            scopePath += part.text();
        }


        ASSERT(currentNamespace);

        //Finally, resolve the element of the identifier in the current scope only.
        scope::Symbol *foundSymbol = currentNamespace->findSymbolInCurrentScope(id.back().text());
        if (foundSymbol == nullptr) {
            errorStream.error(
                error::ErrorKind::NamespaceMemberDoesNotExist,
                id.back().span(),
                "symbol '%s' does not exist in namespace '%s'",
                id.back().text().c_str(),
                scopePath.c_str());
        }
        return foundSymbol;
    }
}

}}