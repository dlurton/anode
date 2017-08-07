

#pragma once
#include "ast.h"

namespace lwnn {
    namespace parse {
        ast::Module *parseModule(const std::string &lineOfCode, const std::string &inputName);
    }
}