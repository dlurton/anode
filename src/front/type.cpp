#include "../include/lwnn/front/type.h"
#include "../include/lwnn/exception.h"
#include <unordered_map>

namespace lwnn {
    namespace type {
        namespace Primitives {
            Type Int32("int", PrimitiveType::Int32, true);
            Type Float("float", PrimitiveType::Float, true);
            Type Double("double", PrimitiveType::Double, true);
            Type Bool("bool", PrimitiveType::Bool, false);
            Type Void("void", PrimitiveType::Void, false);

            Type *fromKeyword(const std::string &keyword) {
                if(keyword == "int") {
                    return &Int32;
                } else if(keyword == "float") {
                    return &Float;
                } else if(keyword == "double") {
                    return &Double;
                } else if(keyword == "bool") {
                    return &Bool;
                } else if(keyword == "void") {
                    return &Void;
                } else {
                    return nullptr;
                }
            }
        }
        std::string to_string(PrimitiveType dataType) {
            switch (dataType) {
                case PrimitiveType::Void:
                    return "void";
                case PrimitiveType::Bool:
                    return "bool";
                case PrimitiveType::Int32:
                    return "int";
                case PrimitiveType::Float:
                    return "float";
                case PrimitiveType::Double:
                    return "double";
                default:
                    ASSERT_FAIL("Unhandled PrimitiveType");
            }
        }
   }
}
