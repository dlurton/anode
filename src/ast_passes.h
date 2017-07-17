
#pragma once

#include "ast.h"
#include "error.h"

#include <deque>

namespace lwnn {
    namespace ast_passes {
        void setSymbolTableParents(ast::AstNode *node);
        void populateSymbolTables(ast::AstNode *node, error::ErrorStream & errorStream);
        void resolveSymbols(ast::AstNode *node, error::ErrorStream & errorStream);
        void resolveTypes(ast::AstNode *node, error::ErrorStream & errorStream);
    }
}