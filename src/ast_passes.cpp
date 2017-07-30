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
                            binaryExpr->operatorSpan(),
                            message,
                            binaryExpr->rValue()->type()->name().c_str(),
                            binaryExpr->lValue()->type()->name().c_str());
                    }
                }
            }

            virtual void visitedIfExpr(ast::IfExpr *condExpr) {

                if(condExpr->condition()->type()->primitiveType() != type::PrimitiveType::Bool) {
                    if (condExpr->condition()->type()->canImplicitCastTo(&type::Primitives::Bool)) {
                        condExpr->graftCondition(
                            [&](std::unique_ptr<ast::ExprStmt> oldTruePart) {
                                return std::make_unique<ast::CastExpr>(
                                    oldTruePart->sourceSpan(),
                                    &type::Primitives::Bool,
                                    std::move(oldTruePart),
                                    ast::CastKind::Implicit);
                            });
                    } else {
                        errorStream_.error(
                            condExpr->condition()->sourceSpan(),
                            "Condition expression cannot be implicitly converted from '%s' to 'bool'.",
                            condExpr->condition()->type()->name().c_str());
                    }
                }


                if(condExpr->thenExpr()->type() != condExpr->elseExpr()->type()) {
                    //If we can implicitly cast the lvalue to same type as the rvalue, we should...
                    if(condExpr->elseExpr()->type()->canImplicitCastTo(condExpr->thenExpr()->type())) {
                        condExpr->graftFalsePart(
                            [&](std::unique_ptr<ast::ExprStmt> oldFalsePart) {
                                return std::make_unique<ast::CastExpr>(
                                    oldFalsePart->sourceSpan(),
                                    condExpr->thenExpr()->type(),
                                    std::move(oldFalsePart),
                                    ast::CastKind::Implicit);
                            });
                    } //otherwise, if we can to the reverse, we should...
                    else if (condExpr->thenExpr()->type()->canImplicitCastTo(condExpr->elseExpr()->type())) {
                        condExpr->graftTruePart(
                            [&](std::unique_ptr<ast::ExprStmt> oldTruePart) {
                                return std::make_unique<ast::CastExpr>(
                                    oldTruePart->sourceSpan(),
                                    condExpr->elseExpr()->type(),
                                    std::move(oldTruePart),
                                    ast::CastKind::Implicit);
                            });
                    }
                    else { // No implicit cast available...
                        errorStream_.error(
                            condExpr->sourceSpan(),
                            "Cannot implicitly convert '%s' to '%s' or vice-versa",
                            condExpr->thenExpr()->type()->name().c_str(),
                            condExpr->elseExpr()->type()->name().c_str());
                    }
                }
            }
        };

        class CastExprSemanticPass : public ast::AstVisitor {
            error::ErrorStream &errorStream_;
        public:
            CastExprSemanticPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }
            virtual void visitingCastExpr(ast::CastExpr *expr) {
                type::Type *fromType = expr->valueExpr()->type();
                type::Type *toType = expr->type();

                if(fromType->canImplicitCastTo(toType)) return;

                if(!fromType->canExplicitCastTo(toType)) {
                    errorStream_.error(
                        expr->sourceSpan(),
                        "Cannot cast from '%s' to '%s'",
                        fromType->name().c_str(),
                        toType->name().c_str());
                }


            }
        };

        class BinaryExprSemanticsPass : public ast::AstVisitor {
            error::ErrorStream &errorStream_;
        public:
            BinaryExprSemanticsPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

            virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) {
                if(binaryExpr->operation() == ast::BinaryOperationKind::Assign) {
                    if(!dynamic_cast<ast::VariableRefExpr*>(binaryExpr->lValue())) {
                        errorStream_.error(binaryExpr->operatorSpan(), "Invalid l-value for operator '='");
                    }
                } else if(ast::isArithmeticOperation(binaryExpr->operation())) {
                    if(!binaryExpr->type()->canDoArithmetic()) {
                        errorStream_.error(binaryExpr->operatorSpan(), "Operator '%s' cannot be used with type '%s'.",
                            ast::to_string(binaryExpr->operation()).c_str(), binaryExpr->type()->name().c_str());
                    }
                }
            }
        };

        void runAllPasses(ast::Module *module, error::ErrorStream &es) {

            std::vector<std::unique_ptr<ast::AstVisitor>> passes;
            passes.emplace_back(std::make_unique<SetSymbolTableParentsAstVisitor>());
            passes.emplace_back(std::make_unique<PopulateSymbolTablesVisitor>(es));
            passes.emplace_back(std::make_unique<TypeResolvingVisitor>(es));
            passes.emplace_back(std::make_unique<SymbolResolvingVisitor>(es));
            passes.emplace_back(std::make_unique<AddImplicitCastsVisitor>(es));
            passes.emplace_back(std::make_unique<BinaryExprSemanticsPass>(es));
            passes.emplace_back(std::make_unique<CastExprSemanticPass>(es));

            for(auto & pass : passes) {
                module->accept(pass.get());
                if(es.errorCount() > 0) {
                    break;
                }
            }
        }
    } //namespace ast_passes
} //namespace lwnn