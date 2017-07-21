
#pragma once

#include "ast.h"
#include "error.h"

#include <deque>

namespace lwnn {
    namespace ast_passes {

        void runAllPasses(ast::Module *module, error::ErrorStream &errorStream);
    }
}