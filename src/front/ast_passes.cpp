#include "front/ast_passes.h"
#include "front/scope.h"
#include "front/error.h"

namespace lwnn {
    namespace ast_passes {

        class ScopeFollowingVisitor : public ast::AstVisitor {

            //We use this only as a stack but it has to be a deque so we can iterate over its contents.
            std::deque<scope::SymbolTable*> symbolTableStack_;
        protected:
            scope::SymbolTable *topScope() {
                ASSERT(symbolTableStack_.size());
                return symbolTableStack_.back();
            }

            size_t scopeDepth() { return symbolTableStack_.size(); }

        public:
            virtual void visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                funcDeclStmt->parameterScope()->setParent(topScope());
                symbolTableStack_.push_back(funcDeclStmt->parameterScope());
            }

            virtual void visitedFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                ASSERT(funcDeclStmt->parameterScope() == symbolTableStack_.back())
                symbolTableStack_.pop_back();
            }

            virtual bool visitingCompoundExpr(ast::CompoundExprStmt *compoundStmt) override {
                symbolTableStack_.push_back(compoundStmt->scope());
                return true;
            }

            virtual void visitedCompoundExpr(ast::CompoundExprStmt *compoundStmt) override {
                ASSERT(compoundStmt->scope() == symbolTableStack_.back());
                symbolTableStack_.pop_back();
            }
        };


        /** Sets each SymbolTable's parent scope. */
        class SetSymbolTableParentsAstVisitor : public ScopeFollowingVisitor {
        public:
            virtual void visitingFuncDeclStmt(ast::FuncDeclStmt *funcDeclStmt) override {
                ScopeFollowingVisitor::visitedFuncDeclStmt(funcDeclStmt);
                funcDeclStmt->parameterScope()->setParent(topScope());
            }

            virtual bool visitingCompoundExpr(ast::CompoundExprStmt *compoundStmt) override {
                //The first entry on the stack would be the global scope which has no parent
                if(scopeDepth()) {
                    compoundStmt->scope()->setParent(topScope());
                }
                ScopeFollowingVisitor::visitingCompoundExpr(compoundStmt);
                return true;
            }
        };

        class PopulateSymbolTablesVisitor : public ScopeFollowingVisitor {
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

        class SymbolResolvingVisitor : public ScopeFollowingVisitor {
            error::ErrorStream &errorStream_;
        public:
            SymbolResolvingVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

            virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) {
                if(expr->symbol()) return;

                scope::Symbol *found = topScope()->recursiveFindSymbol(expr->name());
                if(!found) {
                    errorStream_.error(expr->sourceSpan(), "Variable '%s' was not defined in this scope.",
                        expr->name().c_str());
                } else {
                    expr->setSymbol(found);
                }
            }
        };

        class TypeResolvingVisitor : public ScopeFollowingVisitor {
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
                if(binaryExpr->binaryExprKind() == ast::BinaryExprKind::Logical) {

                    if(binaryExpr->lValue()->type()->primitiveType() != type::PrimitiveType::Bool) {
                        binaryExpr->graftLValue(
                            [&](std::unique_ptr<ast::ExprStmt> oldLValue) {
                                return std::make_unique<ast::CastExpr>(
                                    oldLValue->sourceSpan(),
                                    &type::Primitives::Bool,
                                    std::move(oldLValue),
                                    ast::CastKind::Implicit);
                            });
                    }

                    if(binaryExpr->rValue()->type()->primitiveType() != type::PrimitiveType::Bool) {
                        binaryExpr->graftRValue(
                            [&](std::unique_ptr<ast::ExprStmt> oldRValue) {
                                return std::make_unique<ast::CastExpr>(
                                    oldRValue->sourceSpan(),
                                    &type::Primitives::Bool,
                                    std::move(oldRValue),
                                    ast::CastKind::Implicit);
                            });
                    }
                }
                else if(binaryExpr->lValue()->type() != binaryExpr->rValue()->type()) {

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

            virtual void visitedIfExpr(ast::IfExprStmt *ifExpr) {

                if(ifExpr->condition()->type()->primitiveType() != type::PrimitiveType::Bool) {
                    if (ifExpr->condition()->type()->canImplicitCastTo(&type::Primitives::Bool)) {
                        ifExpr->graftCondition(
                            [&](std::unique_ptr<ast::ExprStmt> oldCondition) {
                                return std::make_unique<ast::CastExpr>(
                                    oldCondition->sourceSpan(),
                                    &type::Primitives::Bool,
                                    std::move(oldCondition),
                                    ast::CastKind::Implicit);
                            });
                    } else {
                        errorStream_.error(
                            ifExpr->condition()->sourceSpan(),
                            "Condition expression cannot be implicitly converted from '%s' to 'bool'.",
                            ifExpr->condition()->type()->name().c_str());
                    }
                }

                if(!ifExpr->elseExpr()) return;

                if(ifExpr->thenExpr()->type() != ifExpr->elseExpr()->type()) {
                    //If we can implicitly cast the lvalue to same type as the rvalue, we should...
                    if(ifExpr->elseExpr()->type()->canImplicitCastTo(ifExpr->thenExpr()->type())) {
                        ifExpr->graftFalsePart(
                            [&](std::unique_ptr<ast::ExprStmt> oldFalsePart) {
                                return std::make_unique<ast::CastExpr>(
                                    oldFalsePart->sourceSpan(),
                                    ifExpr->thenExpr()->type(),
                                    std::move(oldFalsePart),
                                    ast::CastKind::Implicit);
                            });
                    } //otherwise, if we can to the reverse, we should...
                    else if (ifExpr->thenExpr()->type()->canImplicitCastTo(ifExpr->elseExpr()->type())) {
                        ifExpr->graftTruePart(
                            [&](std::unique_ptr<ast::ExprStmt> oldTruePart) {
                                return std::make_unique<ast::CastExpr>(
                                    oldTruePart->sourceSpan(),
                                    ifExpr->elseExpr()->type(),
                                    std::move(oldTruePart),
                                    ast::CastKind::Implicit);
                            });
                    }
                    else { // No implicit cast available...
                        errorStream_.error(
                            ifExpr->sourceSpan(),
                            "Cannot implicitly convert '%s' to '%s' or vice-versa",
                            ifExpr->thenExpr()->type()->name().c_str(),
                            ifExpr->elseExpr()->type()->name().c_str());
                    }
                }
            }

            virtual bool visitingWhileExpr(ast::WhileExpr *whileExpr) {
                //Can we deduplicate this code? (duplicate is in AddImplicitCastsVisitor::visitedIfExpr)
                if(whileExpr->condition()->type()->primitiveType() != type::PrimitiveType::Bool) {
                    if (whileExpr->condition()->type()->canImplicitCastTo(&type::Primitives::Bool)) {
                        whileExpr->graftCondition(
                            [&](std::unique_ptr<ast::ExprStmt> oldCondition) {
                                return std::make_unique<ast::CastExpr>(
                                    oldCondition->sourceSpan(),
                                    &type::Primitives::Bool,
                                    std::move(oldCondition),
                                    ast::CastKind::Implicit);
                            });
                    } else {
                        errorStream_.error(
                            whileExpr->condition()->sourceSpan(),
                            "Condition expression cannot be implicitly converted from '%s' to 'bool'.",
                            whileExpr->condition()->type()->name().c_str());
                    }
                }

                return true;
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
                if(binaryExpr->isComparison()) {
                    return;
                }
                if(binaryExpr->operation() == ast::BinaryOperationKind::Assign) {
                    if(!dynamic_cast<ast::VariableRefExpr*>(binaryExpr->lValue())) {
                        errorStream_.error(binaryExpr->operatorSpan(), "Invalid l-value for operator '='");
                    }
                } else if(binaryExpr->binaryExprKind() == ast::BinaryExprKind::Arithmetic) {
                    if(!binaryExpr->type()->canDoArithmetic()) {
                        errorStream_.error(binaryExpr->operatorSpan(), "Operator '%s' cannot be used with type '%s'.",
                            ast::to_string(binaryExpr->operation()).c_str(), binaryExpr->type()->name().c_str());
                    }
                    //TODO? if(!binaryExpr->type()->canDoLogic()) {
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