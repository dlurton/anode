
#pragma once

#include <string>
#include "common/exception.h"

namespace lwnn {
    namespace type {

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
        public:
            Type(const std::string &name) : name_(name) {}

            virtual std::string name() const { return name_; }
            virtual bool isPrimitive() const = 0;
            virtual PrimitiveType primitiveType() const = 0;
            virtual bool canDoArithmetic() const = 0;
            virtual bool canImplicitCastTo(Type *other) const = 0;
            virtual bool canExplicitCastTo(Type *other) const = 0;
        };

        class ScalarType : public Type {
            PrimitiveType primitiveType_;
            bool canDoArithmetic_;
        public:
            ScalarType(const std::string &name, PrimitiveType primitiveType_, bool canDoArithmetic)
                : Type(name), primitiveType_(primitiveType_), canDoArithmetic_(canDoArithmetic) {}

            virtual bool isPrimitive() const override { return primitiveType() != PrimitiveType::NotAPrimitive; }
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

        class ClassType : public Type {
        public:
            ClassType(const std::string &name) : Type(name) {

            }

            bool isPrimitive() const { return false; }
            PrimitiveType primitiveType() const { return PrimitiveType::NotAPrimitive; }

            bool canDoArithmetic() const { return false; };
            bool canImplicitCastTo(Type *) const { return false; };
            bool canExplicitCastTo(Type *) const { return false; };
        };
    }
}