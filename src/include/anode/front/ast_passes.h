
#pragma once

#include "ast.h"
#include "ErrorStream.h"
#include "common/containers.h"

namespace anode { namespace front  { namespace passes {

    /** Runs all AST passes that are part of the compilation process, except for code generation. */
    void runAllPasses(ast::AnodeWorld &world, ast::Module *module, error::ErrorStream &es);

}}}