
#pragma once

#include "common/exception.h"
#include "common/containers.h"

#include <string>
#include <common/string_format.h>


namespace lwnn { namespace type {

/** Each type of primitive is listed here.
 * IMPORTANT NOTE:  the are listed in order of operand priority (except for the NotAPrimitive and Void values).
 * In other words, in given a binary expression such as "someFloat * someDouble", the result of the expression
 * is a double because double has a higher operand priority than than float.  Similarly, in the expression
 * "someInt * someFloat", the result is a float.
 */
enum class PrimitiveType {
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
    std::string name_;
protected:
    Type(const std::string &name) : name_(name) {}
public:

    /** Name of the type. */
    virtual std::string name() const { return name_; }

    virtual bool isSameType(type::Type *) const = 0;

    /** True if the type is a primitive value (i.e. int, bool, float, etc) */
    virtual bool isPrimitive() const { return primitiveType() != PrimitiveType::NotAPrimitive; };

    /** True if the type is void. */
    virtual bool isVoid() const { return primitiveType() == PrimitiveType::Void; };

    /** True of the type a class. */
    virtual bool isClass() const { return false; };

    /** True if the type is a function and can be invoked.*/
    virtual bool isFunction() const { return false; }

    /** The PrimitiveType enum or PrimitiveType::NotAPrimitive if the type is not a primitive. */
    virtual PrimitiveType primitiveType() const { return PrimitiveType::NotAPrimitive; };

    /** True when arithmetic (add, subtract, mul, div, etc) */
    virtual bool canDoArithmetic() const { return false; }

    /** True if values of this type may be implicitly cast to the other type.*/
    virtual bool canImplicitCastTo(Type *) const { return false; }

    /** True if values of htis type may be explicitly cast to the other type.*/
    virtual bool canExplicitCastTo(Type *) const { return false;}
};

class TypeResolutionListener {
public:
    virtual void notifyTypeResolved(type::Type *type) = 0;
};

class ScalarType : public Type {
    PrimitiveType primitiveType_;
    bool canDoArithmetic_;
public:
    ScalarType(const std::string &name, PrimitiveType primitiveType_, bool canDoArithmetic)
        : Type(name), primitiveType_(primitiveType_), canDoArithmetic_(canDoArithmetic) {}

    bool isSameType(type::Type *other) const {
        //We never create more than once instance of ScalarType for a given scalar data type
        //so this simple check suffices.
        return this == other;
    }

    bool canDoArithmetic() const override { return canDoArithmetic_; }
    PrimitiveType primitiveType() const { return primitiveType_; }
    int operandPriority() const { return (int) primitiveType(); }

    /** Returns true when a value of the specified type can be implicitly cast to this type.
     * Returns false if the other type is the same as this type (as this does not require casting). */
    bool canImplicitCastTo(Type *other) const override  {
        ScalarType *otherScalar = dynamic_cast<ScalarType*>(other);
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
    bool canExplicitCastTo(Type *other) const override {
        ScalarType *otherScalar = dynamic_cast<ScalarType*>(other);

        if(otherScalar == nullptr) return false;

        if(primitiveType_ == otherScalar->primitiveType()) return false;

        if(primitiveType_ == type::PrimitiveType::Bool) return false;

        return operandPriority() >= otherScalar->operandPriority();
    };
};

class FunctionType : public Type {
    Type *returnType_;
public:
    FunctionType(type::Type *returnType) : Type("func:" + returnType->name()), returnType_{returnType} { }
    bool isSameType(type::Type *other) const {
        if(this == other) return true;
        if(!other->isFunction()) return false;

        FunctionType* otherFunctionType = dynamic_cast<FunctionType*>(other);
        return otherFunctionType && this->returnType_->isSameType(otherFunctionType->returnType_);
    }

    bool isFunction() const override { return true; }

    Type *returnType() const { return returnType_; }
};

class ClassField : public type::TypeResolutionListener, public gc {
    type::Type * type_;
    std::string name_;
    unsigned const ordinal_;
public:
    ClassField(const std::string &name, type::Type *type, unsigned ordinal) : type_{type}, name_{name}, ordinal_{ordinal} { }

    type::Type *type() const { ASSERT(type_ && "Type must be resolved first."); return type_; }
    void notifyTypeResolved(type::Type *type) override  { type_ = type; }

    std::string name() const { return name_; }

    unsigned ordinal() const { return ordinal_; }
};

class ClassType : public Type {
    gc_vector<ClassField*> orderedFields_;
    gc_unordered_map<std::string, ClassField*> fields_;
public:
    ClassType(const std::string &name) : Type(name) {

    }

    bool isSameType(type::Type *other) const {
        //We don't expect multiple instances of ClassType to be created which reference the same class
        //so this simple check suffices.
        return other == this;
    }

    bool isClass() const override { return true; }
    PrimitiveType primitiveType() const override { return PrimitiveType::NotAPrimitive; }

    bool canDoArithmetic() const override { return false; };
    bool canImplicitCastTo(Type *) const override { return false; };
    bool canExplicitCastTo(Type *) const override { return false; };

    ClassField *findField(const std::string &name) {
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