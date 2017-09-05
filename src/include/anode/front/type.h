
#pragma once

#include "common/exception.h"
#include "common/containers.h"

#include <string>
#include <common/string_format.h>


namespace anode { namespace type {

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

namespace Primitives {
    extern ScalarType Int32;
    extern ScalarType Float;
    extern ScalarType Double;
    extern ScalarType Bool;
    extern ScalarType Void;

    ScalarType *fromKeyword(const std::string &keyword);
}

class Type : public gc {
public:
    virtual std::string name() const = 0;
    virtual bool isSameType(const type::Type *) const = 0;
    virtual bool isPrimitive() const { return primitiveType() != PrimitiveType::NotAPrimitive; };
    virtual bool isActualType() { return true; }
    virtual bool isVoid() const { return primitiveType() == PrimitiveType::Void; };
    virtual bool isClass() const { return false; };
    virtual bool isFunction() const { return false; }
    virtual PrimitiveType primitiveType() const { return PrimitiveType::NotAPrimitive; };
    virtual bool canDoArithmetic() const { return false; }
    virtual bool canImplicitCastTo(const Type *) const { return false; }
    virtual bool canExplicitCastTo(const Type *) const { return false; }
    virtual Type* actualType() const { return const_cast<Type*>(this); }
};

class ResolutionDeferredType : public Type {
    Type *actualType_ = nullptr;
    void assertResolved() const {
        ASSERT(actualType_ && "ResolutionDeferredType has not been resolved yet.");
    }
public:
    ResolutionDeferredType() { }
    ResolutionDeferredType(Type *actualType) : actualType_{actualType} { }

    virtual bool isActualType() { return false; }

    bool isResolved() { return actualType_ != nullptr; }

    void resolve(Type* type) {
//        ASSERT(! dynamic_cast<ResolutionDeferredType*>(type)
//               && "ResolutionDeferredType should not resolve to an instance of ResolutionDeferredType")

        actualType_ = type;
    }

    Type *actualType() const override {
        assertResolved();

        //TODO:  detect cycles - how to handle them?  Error?

        return actualType_->actualType();
    }

    std::string name() const override {
        assertResolved();
        return actualType()->name();
    }

    bool isSameType(const type::Type *otherType) const override {
        assertResolved();
        return actualType()->isSameType(otherType);
    }

    bool isPrimitive() const override {
        assertResolved();
        return actualType()->isPrimitive();
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

    bool isSameType(const type::Type *other) const {
        //We never create more than once instance of ScalarType for a given scalar data type
        //so this simple check suffices.
        return this == other->actualType();
    }

    bool canDoArithmetic() const override { return canDoArithmetic_; }
    PrimitiveType primitiveType() const { return primitiveType_; }

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
};

class FunctionType : public Type {
    Type *returnType_;
    gc_vector<type::Type*> parameterTypes_;
public:
    FunctionType(Type *returnType, const gc_vector<type::Type*> parameterTypes) : returnType_{returnType}, parameterTypes_{parameterTypes}  { }

    std::string name() const override { return "func:" + returnType_->name(); }

    bool isSameType(const type::Type *other) const {
        if(this == other) return true;
        if(!other->isFunction()) return false;

        auto otherFunctionType = dynamic_cast<const FunctionType*>(other);
        return otherFunctionType && this->returnType_->isSameType(otherFunctionType->returnType_);
    }

    bool isFunction() const override { return true; }

    Type *returnType() const { return returnType_; }
    gc_vector<type::Type*> parameterTypes() {
        gc_vector<type::Type*> retval;
        retval.reserve(parameterTypes_.size());
        for(type::Type *ptype : parameterTypes_) {
            retval.push_back(ptype);
        }
        return retval;
    }
};

class ClassField : public gc {
    type::Type * type_;
    std::string name_;
    unsigned const ordinal_;
public:
    ClassField(const std::string &name, type::Type *type, unsigned ordinal) : type_{type}, name_{name}, ordinal_{ordinal} { }

    type::Type *type() const { return type_; }

    std::string name() const { return name_; }

    unsigned ordinal() const { return ordinal_; }
};

class ClassType : public Type {
    std::string name_;
    gc_vector<ClassField*> orderedFields_;
    gc_unordered_map<std::string, ClassField*> fields_;
public:
    ClassType(const std::string &name) : name_{name} {
        ASSERT(name_.size() > 0);
    }

    std::string name() const override { return name_; }

    bool isSameType(const type::Type *other) const {
        //We don't expect multiple instances of ClassType to be created which reference the same class
        //so this simple check suffices.
        return other->actualType() == this;
    }

    bool isClass() const override { return true; }
    PrimitiveType primitiveType() const override { return PrimitiveType::NotAPrimitive; }

    bool canDoArithmetic() const override { return false; };
    bool canImplicitCastTo(const Type *) const override { return false; };
    bool canExplicitCastTo(const Type *) const override { return false; };

    ClassField *findField(const std::string &name) const {
        auto found = fields_.find(name);
        return found == fields_.end() ? nullptr : found->second;
    }

    ClassField &addField(const std::string &name) {
        return addField(name, nullptr);
    }

    ClassField &addField(const std::string &name, type::Type *type) {
        auto field = new ClassField(name, type, (unsigned) orderedFields_.size());

        orderedFields_.emplace_back(field);
        fields_.emplace(name, field);

        return *field;
    }
};
}}