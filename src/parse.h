

#pragma onceonce
#include "ast.h"

namespace lwnn {
    namespace parse {
        extern std::unique_ptr<ast::Module> parseModule(std::string lineOfCode);
    }
}