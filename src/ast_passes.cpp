#include "ast_passes.h"
#include "scope.h"
#include "error.h"

namespace lwnn {
    namespace ast_passes {

        /** Sets each SymbolTable's parent scope. */
        class SetSymbolTableParentsAstVisitor : public ast::ScopeFollowingVisitor {
        public:
            virtual void visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                ast::ScopeFollowingVisitor::visitedFuncDeclStmt(funcDeclStmt);
                funcDeclStmt->parameterScope()->setParent(topScope());
            }

            virtual void visitingCompoundStmt(ast::CompoundStmt *compoundStmt) override {
                ast::ScopeFollowingVisitor::visitingCompoundStmt(compoundStmt);
                compoundStmt->scope()->setParent(topScope());
            }
        };

        class PopulateSymbolTablesVisitor : public ast::ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:

            PopulateSymbolTablesVisitor(error::ErrorStream &errorStream) : errorStream_(errorStream) {

            }

            virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
                if(topScope()->findSymbol(expr->name())) {
                    errorStream_.error(expr->sourceSpan(), "Symbol '%s' is already defined in this scope.",
                                       expr->name().c_str());
                } else {
                    topScope()->addSymbol(expr);
                }
            }
        };

        class SymbolResolvingVisitor : public ast::ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:
            SymbolResolvingVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

            virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) {
                if(expr->symbol()) return;

                scope::Symbol *found = topScope()->findSymbol(expr->name());
                if(!found) {
                    errorStream_.error(expr->sourceSpan(), "Variable '%s' was not defined in this scope.",
                        expr->name().c_str());
                } else {
                    expr->setSymbol(found);
                }

            }
        };
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

        class AddImplicitCastsVisitor : public ast::AstVisitor {
            error::ErrorStream &errorStream_;
        public:
            AddImplicitCastsVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

            virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) {

                if(binaryExpr->lValue()->type() != binaryExpr->rValue()->type())
                {
                    //If we can implicitly cast the lvalue to same type as the rvalue, we should...
                    if(binaryExpr->lValue()->type()->canImplicitCastTo(binaryExpr->rValue()->type())
                       && binaryExpr->operation() != ast::BinaryOperationKind::Assign) {
                        binaryExpr->graftLValue(
                            [&](std::unique_ptr<ast::ExprStmt> oldLValue) {
                                return std::make_unique<ast::CastExpr>(
                                    oldLValue->sourceSpan(),
                                    binaryExpr->rValue()->type(),
                                    std::move(oldLValue),
                                    ast::CastKind::Implicit);
                            });
                    } //otherwise, if we can to the reverse, we should...
                    else if (binaryExpr->rValue()->type()->canImplicitCastTo(binaryExpr->lValue()->type())) {
                        binaryExpr->graftRValue(
                            [&](std::unique_ptr<ast::ExprStmt> oldRValue) {
                                return std::make_unique<ast::CastExpr>(
                                    oldRValue->sourceSpan(),
                                    binaryExpr->lValue()->type(),
                                    std::move(oldRValue),
                                    ast::CastKind::Implicit);
                            });
                    }
                    else { // No implicit cast available...
                        const char *message = binaryExpr->operation() == ast::BinaryOperationKind::Assign
                                        ? "Cannot assign value of type '%s' to a variable of type '%s'"
                                        : "Cannot implicitly convert '%s' to '%s' or vice-versa";

                        errorStream_.error(
                            //<-- it might be clearest if we called out the location of the operator here, however we don't have that at this time...
                            binaryExpr->sourceSpan(),
                            message,
                            binaryExpr->rValue()->type()->name().c_str(),
                            binaryExpr->lValue()->type()->name().c_str());
                    }
                }
            }
//
//            virtual void visitedVariableDeclExpr(ast::VariableDeclExpr *varDeclExpr) override {
//                type::Type *varDeclType = varDeclExpr->type();
//                if(!varDeclExpr->initializerExpr())
//                    return;
//
//                type::Type *initializerType = varDeclExpr->initializerExpr()->type();
//
//                //If data type of initializerExpr doesn't already match that of the variable being declared...
//                if(varDeclType != initializerType) {
//                    //If the initializerExpr can be implicitly cast to the type of the variable being declared...
//                    if(initializerType->canImplicitCastTo(varDeclType)) {
//                        varDeclExpr->graftInitializerExpr([&](std::unique_ptr<ast::ExprStmt> oldInitializerExpr) {
//                            return std::make_unique<ast::CastExpr>(
//                                oldInitializerExpr->sourceSpan(),
//                                varDeclType,
//                                std::move(oldInitializerExpr),
//                                ast::CastKind::Implicit);
//                        });
//                    } else {
//                        errorStream_.error(
//                            //<-- it might be clearest if we called out the location of the operator here, however we don't have that at this time...
//                            varDeclExpr->initializerExpr()->sourceSpan(),
//                            "Cannot implicitly convert '%s' to '%s'",
//                            initializerType->name().c_str(),
//                            varDeclType->name().c_str());
//                    }
//                }
//            }
        };
        class MarkAssignmentLValuesAstVisitor : public ast::AstVisitor {
        public:
            virtual void visitingBinaryExpr(ast::BinaryExpr *expr) override {
                if(expr->operation() != ast::BinaryOperationKind::Assign) return;
                if(ast::VariableRefExpr *varRefExpr = dynamic_cast<ast::VariableRefExpr*>(expr->lValue())) {
                    varRefExpr->setVariableAccess(ast::VariableAccess::Write);
                }
            }
//            virtual void visitedBinaryExpr(ast::BinaryExpr *expr) override {
//
//            }
        };

        void runAllPasses(ast::Module *module, error::ErrorStream &es) {
            SetSymbolTableParentsAstVisitor setSymbolTableParentsAstVisitor;
            module->accept(&setSymbolTableParentsAstVisitor);

            PopulateSymbolTablesVisitor populateSymbolTablesVisitor{es};
            module->accept(&populateSymbolTablesVisitor);
            if(es.errorCount() > 0)
                return;

            TypeResolvingVisitor resolvingVisitor{es};
            module->accept(&resolvingVisitor);
            if(es.errorCount() > 0)
                return;

            SymbolResolvingVisitor symbolResolvingVisitor{es};
            module->accept(&symbolResolvingVisitor);
            if(es.errorCount() > 0)
                return;

            AddImplicitCastsVisitor addImplicitCastsVisitor{es};
            module->accept(&addImplicitCastsVisitor);
            if(es.errorCount() > 0)
                return;

            MarkAssignmentLValuesAstVisitor markAssignmentLValuesAstVisitor;
            module->accept(&markAssignmentLValuesAstVisitor);
        }
    } //namespace ast_passes
} //namespace lwnn