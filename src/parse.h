

#pragma onceonce
#include "ast.h"

namespace lwnn {
    namespace parse {
        extern std::unique_ptr<const ast::Expr> parseString(std::string lineOfCode);
    }
}