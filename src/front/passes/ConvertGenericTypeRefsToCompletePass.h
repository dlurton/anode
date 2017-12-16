
#pragma once

#include "ErrorContextAstVisitor.h"

namespace anode { namespace front  { namespace passes {

/**
 * After symbol table resolution reference to generic types have been resolved to GenericType instances.
 * This this visitor will look up the type::ClassType which was added to the type::GenericType instances during
 * PopulateGenericTypesWithCompleteTypesPass.
 */
class ConvertGenericTypeRefsToCompletePass : public ErrorContextAstVisitor {
public:
    ConvertGenericTypeRefsToCompletePass(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef &typeRef) override {
        if(typeRef.type().isGeneric()) {
            gc_ref_vector<type::Type> templateArgs = typeRef.resolutionDeferredType()->typeArguments();
            auto genericType = upcast<type::GenericType>(typeRef.type().actualType());
            if(genericType->templateParameterCount() != (int)templateArgs.size()) {
                errorStream_.error(
                    error::ErrorKind::IncorrectNumberOfGenericArguments,
                    typeRef.sourceSpan(),
                    "Incorrect number of generic arguments for type '%s' - expected %d but found %d",
                    genericType->nameForDisplay().c_str(),
                    genericType->templateParameterCount(),
                    templateArgs.size());
                return;
            }

            type::ClassType *expandedType = genericType->findExpandedClassType(templateArgs);
            if(!expandedType) {
                errorStream_.error(
                    error::ErrorKind::GenericTypeWasNotExpandedWithSpecifiedArguments,
                    typeRef.sourceSpan(),
                    "Generic type '%s' was not expanded with the specified type arguments",
                    genericType->nameForDisplay().c_str());
                return;
            }

            typeRef.setType(*expandedType);

            } else if(typeRef.hasTemplateArguments()) {
                errorStream_.error(
                    error::ErrorKind::TypeIsNotGenericButIsReferencedWithGenericArgs,
                    typeRef.sourceSpan(),
                    "Type '%s' is not generic but is referenced with generic arguments",
                    typeRef.type().nameForDisplay().c_str());
            }
    }
};


}}}