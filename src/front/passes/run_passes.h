
#pragma once

#include "front/ErrorStream.h"
#include "front/ast.h"

namespace anode { namespace front { namespace passes {

bool runPasses(const gc_ref_vector<ast::AstVisitor> &visitors, ast::AstNode &node, error::ErrorStream &es, scope::SymbolTable *startingSymbolTable);

inline bool runPasses(const gc_ref_vector<ast::AstVisitor> &visitors, ast::AstNode &node, error::ErrorStream &es, scope::SymbolTable &startingSymbolTable) {
    return runPasses(visitors, node, es, &startingSymbolTable);
}

inline bool runPasses(const gc_ref_vector<ast::AstVisitor> &visitors, ast::AstNode &node, error::ErrorStream &es) {
    return runPasses(visitors, node, es, nullptr);
}
gc_ref_vector<ast::AstVisitor> getPreTemplateExpansionPassses(ast::AnodeWorld &world, error::ErrorStream &es);

}}}