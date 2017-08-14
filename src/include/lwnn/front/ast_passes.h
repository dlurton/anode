
#pragma once

#include "ast.h"
#include "error.h"


namespace lwnn { namespace front  { namespace passes {

    /** Runs all AST passes that are part of the compilation process, except for code generation. */
    void runAllPasses(ast::Module *module, error::ErrorStream &errorStream);

}}}