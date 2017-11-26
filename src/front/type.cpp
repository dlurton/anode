#include "front/type.h"
#include "front/scope.h"
#include "common/exception.h"

namespace anode { namespace front { namespace type {

UnresolvedType UnresolvedType::Instance;

ScalarType ScalarType::Int32("int", PrimitiveType::Int32, true);
ScalarType ScalarType::Float("float", PrimitiveType::Float, true);
ScalarType ScalarType::Double("double", PrimitiveType::Double, true);
ScalarType ScalarType::Bool("bool", PrimitiveType::Bool, false);
ScalarType ScalarType::Void("void", PrimitiveType::Void, false);

ScalarType *ScalarType::fromKeyword(const std::string &keyword) {
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
            return "<unresolved>";
        default:
            ASSERT_FAIL("Unhandled PrimitiveType");
    }
}

Type &ClassMethod::type() const { return symbol_.type(); }

void ClassType::addMethod(const std::string &name, scope::FunctionSymbol &symbol) {
    methods_.emplace(name, *new ClassMethod(name, symbol));
}


}}}
