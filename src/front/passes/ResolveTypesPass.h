
#pragma once

#include "ScopeFollowingAstVisitor.h"
#include "symbol_search.h"

namespace anode { namespace front  { namespace passes {


class ResolveTypesPass : public ScopeFollowingAstVisitor {
public:
    explicit ResolveTypesPass(error::ErrorStream &errorStream)
        : ScopeFollowingAstVisitor(errorStream) {
    }

    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef &typeRef) override {
        if(typeRef.isResolved()) {
            return;
        }
        auto &resolvedName = typeRef.name();
        type::Type *type = nullptr;

        if(resolvedName.size() == 1) {
            type = type::ScalarType::fromKeyword(resolvedName.front().text());
            if(type) {
                typeRef.setType(*type);
                return;
            }
        }

        scope::Symbol* maybeType = findQualifiedSymbol(topScope(),  typeRef.name(), errorStream_);

        //Symbol doesn't exist in accessible scope?
        if(!maybeType) {
            //Error message handled by findQualifiedSymbol(...)
            return;
        }

        auto *typeSymbol = dynamic_cast<scope::TypeSymbol*>(maybeType);

        //Symbol does exist but isn't a type.
        if(typeSymbol == nullptr) {
            errorStream_.error(error::ErrorKind::SymbolIsNotAType, typeRef.sourceSpan(),
                               "Symbol '%s' is not a type.", typeRef.name().qualifedName().c_str());
            return;
        }

        typeRef.setType(typeSymbol->type());
    }
};


}}}