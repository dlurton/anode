
#pragma once
#include "front/scope.h"
#include "front/ast.h"
#include "front/ErrorStream.h"

namespace anode { namespace front {
extern scope::Symbol *findQualifiedSymbol(scope::SymbolTable &startingScope, const ast::MultiPartIdentifier &id, error::ErrorStream &errorStream);

}}