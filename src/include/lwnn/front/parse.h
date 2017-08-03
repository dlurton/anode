

#pragma once
#include "ast.h"

namespace lwnn {
    namespace parse {
        std::unique_ptr<ast::Module> parseModule(const std::string &lineOfCode, const std::string &inputName);
    }
}