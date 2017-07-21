
#pragma once

#include <string>
#include "exception.h"

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

        class Type {
            std::string name_;
            PrimitiveType primitiveType_;
        public:
            Type(const std::string &name_, PrimitiveType primitiveType_)
                : name_(name_), primitiveType_(primitiveType_) {}

            bool isPrimitive() const { return primitiveType() != PrimitiveType::NotAPrimitive; }

            std::string name() const { return name_; }
            PrimitiveType primitiveType() const { return primitiveType_; }

            int operandPriority() const { return (int) primitiveType(); }

            bool canImplicitCastTo(Type *other) const {
                return operandPriority() <= other->operandPriority();
            }

            bool canExplicitCastTo(Type *other) const {
                return operandPriority() >= other->operandPriority();
            };
        };

        namespace Primitives {
            extern Type Int32;
            extern Type Float;
            extern Type Double;
            extern Type Bool;
            extern Type Void;

            Type *fromKeyword(const std::string &keyword);
        }
    }
}