

#pragma onceonce
#include "ast.h"

namespace lwnn {
    extern std::unique_ptr<const Expr> parse(std::string lineOfCode);
}