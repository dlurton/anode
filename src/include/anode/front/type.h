
#pragma once
#include "unique_id.h"
#include "common/exception.h"
#include "common/containers.h"

#include "common/string.h"


namespace anode { namespace front { namespace scope {
class Symbol;
    class FunctionSymbol;
}}}

namespace anode { namespace front { namespace type {

/** Each type of primitive is listed here.
 * IMPORTANT NOTE:  the are listed in order of operand priority (except for the NotAPrimitive and Void values).
 * In other words, in given a binary expression such as "someFloat * someDouble", the result of the expression
 * is a double because double has a higher operand priority than than float.  Similarly, in the expression
 * "someInt * someFloat", the result is a float.
 */
enum class PrimitiveType : unsigned char {
    NotAPrimitive,
    Void,
    Bool,
    Int32,
    Float,
    Double
};
std::string to_string(PrimitiveType dataType);

class ScalarType;
class GenericType;
class ClassType;

class Type : public Object {
public:
    NO_COPY_NO_ASSIGN(Type)
    Type() {}
    virtual UniqueId astNodeId() const { return (UniqueId)-1; }
    virtual std::string name() const = 0;
    virtual std::string nameForDisplay() const { return name(); }

    virtual bool isSameType(const type::Type *) const = 0;
    virtual bool isSameType(const type::Type &other) const { return this->isSameType(&other); }
    virtual bool isPrimitive() const { return primitiveType() != PrimitiveType::NotAPrimitive; };
    virtual bool isGeneric() const { return false; }
    virtual bool isActualType() { return true; }
    virtual bool isVoid() const { return primitiveType() == PrimitiveType::Void; };
    virtual bool isClass() const { return false; };
    virtual bool isFunction() const { return false; }
    virtual PrimitiveType primitiveType() const { return PrimitiveType::NotAPrimitive; };
    virtual bool canDoArithmetic() const { return false; }

    virtual bool canImplicitCastTo(const Type &other) const { return this->canImplicitCastTo(&other); }
    virtual bool canExplicitCastTo(const Type &other) const { return this->canExplicitCastTo(&other); }

    virtual bool canImplicitCastTo(const Type *) const { return false; }
    virtual bool canExplicitCastTo(const Type *) const { return false; }
    virtual Type* actualType() const { return const_cast<Type*>(this); }
};

/**
 * UnresolvedType is a special placeholder type for types that haven't yet been resolved.
 * This allows attempts to access the type of a unresolved TypeRef succeed, even if the type isn't capable of
 * participating in any expression in a meaningful way.
 * This type can't participate in arithmetic, and can't be implicitly or explicitly cast to any other type, and
 * is never considered to be the type as another type, even when compared to itself.
 */
class UnresolvedType : public Type {
public:
    UniqueId astNodeId() const override { return (UniqueId)-1; }
    std::string name() const override { return "<unresolved type>"; }
    std::string nameForDisplay() const override { return "<unresolved type>"; }

    bool isSameType(const type::Type *) const override { return false; }
    bool isPrimitive() const override { return primitiveType() != PrimitiveType::NotAPrimitive; };
    bool isGeneric() const override { return false; }
    bool isActualType() override { return true; }
    bool isVoid() const override { return false; };
    bool isClass() const override { return false; };
    bool isFunction() const override { return false; }
    PrimitiveType primitiveType() const override { return PrimitiveType::NotAPrimitive; };
    bool canDoArithmetic() const override { return false; }
    bool canImplicitCastTo(const Type *) const override { return false; }
    bool canExplicitCastTo(const Type *) const override { return false; }
    Type* actualType() const override { return const_cast<UnresolvedType*>(this); }

    static UnresolvedType Instance;
};

class ResolutionDeferredType : public Type {
    Type *actualType_ = &UnresolvedType::Instance;
    gc_ref_vector<Type> typeArguments_;
    void assertResolved() const {
        ASSERT(isResolved() && "ResolutionDeferredType has not been resolved yet.");
    }
public:
    ResolutionDeferredType(const gc_ref_vector<Type> &typeArguments)
        : typeArguments_{typeArguments} { }

    bool isActualType() override { return false; }

    bool isResolved() const {
        if(actualType_ == &UnresolvedType::Instance) {
            return false;
        }

        //Have an actual type that is another ResolutionDeferredType?  Ask it instead.
        if(auto rdt = dynamic_cast<ResolutionDeferredType*>(actualType_)) {
            return rdt->isResolved();
        }

        //We definitively know that this type has been resolved.
        return true;
    }

    gc_ref_vector<type::Type> typeArguments() { return typeArguments_; }

    void resolve(Type* type) {
#ifdef ANODE_DEBUG
        if(isResolved() && (!isInstanceOf<GenericType>(actualType_) || !isInstanceOf<ClassType>(type))) {
            ASSERT_FAIL("Something is probably wrong if you're attempting to resolve this instance of ResolutionDeferredType more than once!");
        }
#endif
        ASSERT(type != this && "Are you trying to cause an infinite loop?  Because that's how you cause an infinite loop.")
        actualType_ = type;
    }

    Type *actualType() const override {
        assertResolved();
        return actualType_->actualType();
    }

    std::string name() const override {
        assertResolved();
        return actualType()->name();
    }

    std::string nameForDisplay() const override {
        assertResolved();
        return actualType_->nameForDisplay();
    }

    bool isSameType(const type::Type *otherType) const override {
        assertResolved();
        return actualType()->isSameType(otherType);
    }

    bool isPrimitive() const override {
        assertResolved();
        return actualType()->isPrimitive();
    }

    virtual bool isGeneric() const override {
        return actualType()->isGeneric();
    }

    bool isVoid() const override {
        assertResolved();
        return actualType()->isVoid();
    }

    bool isClass() const override {
        assertResolved();
        return actualType()->isClass();
    }

    bool isFunction() const override {
        assertResolved();
        return actualType()->isFunction();
    }

    PrimitiveType primitiveType() const override {
        assertResolved();
        return actualType()->primitiveType();
    }

    bool canDoArithmetic() const override {
        assertResolved();
        return actualType()->canDoArithmetic();
    }

    bool canImplicitCastTo(const Type *otherType) const override {
        assertResolved();
        return actualType()->canImplicitCastTo(otherType);
    }

    bool canExplicitCastTo(const Type *otherType) const override {
        assertResolved();
        return actualType()->canExplicitCastTo(otherType);
    }
};

class ScalarType : public Type {
    std::string name_;
    PrimitiveType primitiveType_;
    bool canDoArithmetic_;
public:
    ScalarType(const std::string &name, PrimitiveType primitiveType_, bool canDoArithmetic)
        : name_{name}, primitiveType_(primitiveType_), canDoArithmetic_(canDoArithmetic) {}

    std::string name() const override { return name_; }

    bool isSameType(const type::Type *other) const override {
        //We never create more than once instance of ScalarType for a given scalar data type
        //so this simple check suffices.
        return this == other->actualType();
    }

    bool canDoArithmetic() const override { return canDoArithmetic_; }
    PrimitiveType primitiveType() const override { return primitiveType_; }

    int operandPriority() const { return (int) primitiveType(); }

    /** Returns true when a value of the specified type can be implicitly cast to this type.
     * Returns false if the other type is the same as this type (as this does not require casting). */
    bool canImplicitCastTo(const Type *other) const override  {
        auto otherScalar = dynamic_cast<const ScalarType*>(other->actualType());
        if(otherScalar == nullptr) return false;

        if(primitiveType_ == otherScalar->primitiveType()) return false;

        if(primitiveType_ == type::PrimitiveType::Bool)
            return false;

        if(isPrimitive() && otherScalar->primitiveType() == type::PrimitiveType::Bool)
            return true;

        return operandPriority() <= otherScalar->operandPriority();
    }

    /** Returns true when a value of the specified type may be be explicitly cast to this type.
     * Returns false if the other type is the same as this type (as this does not require casting). */
    bool canExplicitCastTo(const Type *other) const override {
        auto otherScalar = dynamic_cast<const ScalarType*>(other->actualType());

        if(otherScalar == nullptr) return false;

        if(primitiveType_ == otherScalar->primitiveType()) return false;

        if(primitiveType_ == type::PrimitiveType::Bool) return false;

        return operandPriority() >= otherScalar->operandPriority();
    };

    static ScalarType Int32;
    static ScalarType Float;
    static ScalarType Double;
    static ScalarType Bool;
    static ScalarType Void;
    static ScalarType *fromKeyword(const std::string &keyword);
};

class FunctionType : public Type {
    Type *returnType_;
    gc_ref_vector<type::Type> parameterTypes_;
public:
    FunctionType(Type *returnType, const gc_ref_vector<type::Type> parameterTypes) : returnType_{returnType}, parameterTypes_{parameterTypes}  { }

    std::string name() const override { return "func:" + returnType_->name(); }

    bool isSameType(const type::Type *other) const override {
        if(this == other) return true;
        if(!other->isFunction()) return false;

        auto otherFunctionType = dynamic_cast<const FunctionType*>(other);
        return otherFunctionType && this->returnType_->isSameType(otherFunctionType->returnType_);
    }

    bool isFunction() const override { return true; }

    Type *returnType() const { return returnType_; }

    const gc_ref_vector<type::Type> &parameterTypes() {
        return parameterTypes_;
    }
};

class ClassMember : public gc {
    std::string name_;
public:
    explicit ClassMember(const std::string &name) : name_(name) {}

    virtual Type &type() const = 0;

    std::string name() const { return name_; }
};

class ClassField : public ClassMember {
    unsigned const ordinal_;
    type::Type &type_;
public:
    explicit ClassField(const std::string &name, type::Type &type, unsigned ordinal)
        : ClassMember(name), ordinal_{ordinal}, type_(type) { }

    unsigned ordinal() const { return ordinal_; }
    Type &type() const override { return type_; }
};

class ClassMethod : public ClassMember {
    scope::FunctionSymbol &symbol_;
public:
    explicit ClassMethod(const std::string &name, scope::FunctionSymbol &symbol) : ClassMember(name), symbol_{symbol} { }
    Type &type() const override;
    scope::FunctionSymbol &symbol() const { return symbol_; }
};


class GenericType;

class ClassType : public Type {
    const UniqueId astNodeId_;
    std::string name_;
    gc_ref_vector<ClassField> orderedFields_;
    gc_ref_unordered_map<std::string, ClassField> fields_;
    gc_ref_unordered_map<std::string, ClassMethod> methods_;
    GenericType *genericType_ = nullptr;
    gc_ref_vector<Type> typeArguments_;

public:
    ClassType(UniqueId astNodeId, const std::string &name, gc_ref_vector<Type> typeArguments)
        : astNodeId_{astNodeId},
          name_{name},
          typeArguments_{typeArguments}
    {
        ASSERT(!name_.empty());
    }

    UniqueId astNodeId() const override { return astNodeId_; }
    std::string name() const override { return name_; }

    std::string nameForDisplay() const override {
        std::string className = name_;

        if(!typeArguments_.empty()) {
            className += '<';

            auto itr = typeArguments_.begin();
            className += (*itr++).get().nameForDisplay();
            for(; itr != typeArguments_.end(); ++itr) {
                className += ", ";
                className += (*itr).get().nameForDisplay();
            }
            className += '>';
        }
        return className;
    }

    bool isSameType(const type::Type *other) const override {
        //We don't expect multiple instances of ClassType to be created which reference the same class
        //so this simple check suffices.
        return other->actualType() == this->actualType();
    }

    bool isClass() const override { return true; }
    GenericType *genericType() { return genericType_; }
    void setGenericType(GenericType *genericType) { genericType_ = genericType; }

    PrimitiveType primitiveType() const override { return PrimitiveType::NotAPrimitive; }

    bool canDoArithmetic() const override { return false; };
    bool canImplicitCastTo(const Type *) const override { return false; };
    bool canExplicitCastTo(const Type *) const override { return false; };

    ClassField *findField(const std::string &name) const {
        auto &&found = fields_.find(name);
        return found == fields_.end() ? nullptr : &found->second.get();
    }

    void addField(const std::string &name, type::Type &type) {
        auto &&field = *new ClassField(name, type, (unsigned) orderedFields_.size());

        orderedFields_.emplace_back(field);
        fields_.emplace(name, field);
    }

    gc_ref_vector<ClassField> fields() {
        return orderedFields_;
    }

    ClassMethod *findMethod(const std::string &name) const {
        auto found = methods_.find(name);
        return found == methods_.end() ? nullptr : &found->second.get();
    }

    void addMethod(const std::string &name, scope::FunctionSymbol &symbol);
};

class ExpandedClassEntry {
    gc_ref_vector<type::Type> templateArgs_;
    type::ClassType &classType_;
public:
    ExpandedClassEntry(gc_ref_vector<type::Type> templateArgs, ClassType &classType_)
        : templateArgs_{templateArgs}, classType_(classType_) { }

    type::ClassType& classType() const { return classType_; }

    bool templateArgsMatch(const gc_ref_vector<type::Type> &otherArgs) const {
        ASSERT(templateArgs_.size() == otherArgs.size());

        // O(n)... could be optimized.
        for(size_t i = 0; i < templateArgs_.size(); ++i) {
            if(!templateArgs_[i].get().isSameType(otherArgs[i])) {
                return false;
            }
        }
        return true;
    }
};

class GenericType : public Type {
    UniqueId astNodeId_;
    std::string name_;
    std::vector<std::string> templateParameterNames_;
    gc_vector<ExpandedClassEntry> expandedClasses_;
public:
    GenericType(UniqueId astNodeId, std::string name, std::vector<std::string> templateParameterNames)
        : astNodeId_{astNodeId}, name_{name}, templateParameterNames_{templateParameterNames}
    { }

    virtual UniqueId astNodeId() const override { return astNodeId_; }
    std::string name() const override { return name_; }
    bool isSameType(const type::Type *otherType) const override { return this == otherType; };
    virtual bool isGeneric() const override { return true; }

    std::vector<std::string> templateParameterNames() { return templateParameterNames_; }
    int templateParameterCount() { return (int) templateParameterNames_.size(); }

    ClassType *findExpandedClassType(const gc_ref_vector<type::Type> &templateArgs) {
        // O(n)... could also be optimized
        for(const auto &entry : expandedClasses_) {
            if(entry.templateArgsMatch(templateArgs)) {
               return &entry.classType();
            }
        }
        return nullptr;
    }

    void addExpandedClass(const gc_ref_vector<type::Type> &templateArgs, type::ClassType &classType) {
        expandedClasses_.emplace_back(templateArgs, classType);
    }
};

}}}