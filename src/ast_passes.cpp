#include "ast_passes.h"
#include "scope.h"
#include "error.h"

namespace lwnn {
    namespace ast_passes {
        class SymbolTableAnnotation {

        };

        class SymbolAnnotation {
            scope::Symbol *symbol;
        };

        /** Sets each SymbolTable's parent scope. */
        class SetSymbolTableParentsAstVisitor : public ast::ScopeFollowingVisitor {
        public:
            virtual bool visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                funcDeclStmt->parameterScope()->setParent(topScope());
                return true;
            }

            virtual bool visitingCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                compoundStmt->scope()->setParent(topScope());
                return true;
            }
        };

        void setSymbolTableParents(ast::AstNode *node) {
            SetSymbolTableParentsAstVisitor visitor;
            node->accept(&visitor);
        }


        class PopulateSymbolTablesVisitor : public ast::ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:

            PopulateSymbolTablesVisitor(error::ErrorStream &errorStream) : errorStream_(errorStream) {

            }

            virtual bool visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
                if(topScope()->findSymbol(expr->name())) {
                    errorStream_.error(expr->sourceSpan(), "Symbol '%s' is already defined in this scope.",
                                       expr->name().c_str());
                }
                topScope()->addSymbol(expr);
                return true;
            }
        };

        void populateSymbolTables(ast::AstNode *node, error::ErrorStream & errorStream) {
            PopulateSymbolTablesVisitor visitor{errorStream};
            node->accept(&visitor);
        }

        class SymbolResolvingVisitor : public ast::ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:
            SymbolResolvingVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

            virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) {
                scope::Symbol *found = topScope()->findSymbol(expr->name());
                if(!found) {
                    errorStream_.error(expr->sourceSpan(), "Variable '%s' was not defined in this scope.",
                        expr->name().c_str());
                }
            }
        };

        void resolveSymbols(ast::AstNode *node, error::ErrorStream & errorStream) {
            SymbolResolvingVisitor visitor{errorStream};
            node->accept(&visitor);
        }

        class TypeResolvingVisitor : public ast::ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:
            TypeResolvingVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) {

            }

            virtual void visitTypeRef(ast::TypeRef *typeRef) {
                //This will eventually be a lot more sophisticated than this
                type::Type* type = type::Primitives::fromKeyword(typeRef->name());
                if(!type) {
                    errorStream_.error(typeRef->sourceSpan(), "Undefined type: '%s'", typeRef->name().c_str());
                }
                typeRef->setType(type);
            }
        };

        void resolveTypes(ast::AstNode *node, error::ErrorStream & errorStream) {
            TypeResolvingVisitor visitor{errorStream};
            node->accept(&visitor);
        }
    }
}