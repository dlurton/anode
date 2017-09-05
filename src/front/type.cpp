#include "front/type.h"
#include "common/exception.h"

namespace anode { namespace type {

namespace Primitives {
    ScalarType Int32("int", PrimitiveType::Int32, true);
    ScalarType Float("float", PrimitiveType::Float, true);
    ScalarType Double("double", PrimitiveType::Double, true);
    ScalarType Bool("bool", PrimitiveType::Bool, false);
    ScalarType Void("void", PrimitiveType::Void, false);

    ScalarType *fromKeyword(const std::string &keyword) {
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

}}
