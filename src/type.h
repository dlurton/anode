
#pragma once

#include <string>

namespace lwnn {
    namespace type {
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
            const std::string name_;
            const PrimitiveType primitiveType_;
        public:
            Type(std::string name, PrimitiveType primitiveType)
                : name_(name), primitiveType_(primitiveType) {}

            std::string name() const { return name_; }

            bool isPrimitive() const { return primitiveType_ != PrimitiveType::NotAPrimitive; }

            PrimitiveType primitiveType() const { return primitiveType_; }
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