
#pragma once

#include <string>
#include "../exception.h"

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

        class Type;

        namespace Primitives {
            extern Type Int32;
            extern Type Float;
            extern Type Double;
            extern Type Bool;
            extern Type Void;

            Type *fromKeyword(const std::string &keyword);
        }

        class Type {
            std::string name_;
            PrimitiveType primitiveType_;
            bool canDoArithmetic_;
        public:
            Type(const std::string &name_, PrimitiveType primitiveType_, bool canDoArithmetic)
                : name_(name_), primitiveType_(primitiveType_), canDoArithmetic_(canDoArithmetic) {}

            bool isPrimitive() const { return primitiveType() != PrimitiveType::NotAPrimitive; }

            bool canDoArithmetic() { return canDoArithmetic_; }

            std::string name() const { return name_; }
            PrimitiveType primitiveType() const { return primitiveType_; }

            int operandPriority() const { return (int) primitiveType(); }

            /** Returns true when a value of the specified type can be implicitly cast to this type.
             * Returns false if the other type is the same as this type (as this does not require casting). */
            bool canImplicitCastTo(Type *other) const {
                if(primitiveType_ == other->primitiveType()) return false;

                if(primitiveType_ == type::PrimitiveType::Bool)
                    return false;

                if(isPrimitive() && other->primitiveType() == type::PrimitiveType::Bool)
                    return true;

                return operandPriority() <= other->operandPriority();
            }

            /** Returns true when a value of the specified type may be be explicitly cast to this type.
             * Returns false if the other type is the same as this type (as this does not require casting). */
            bool canExplicitCastTo(Type *other) const {
                if(primitiveType_ == other->primitiveType()) return false;

                if(primitiveType_ == type::PrimitiveType::Bool) return false;

                return operandPriority() >= other->operandPriority();
            };
        };

    }
}