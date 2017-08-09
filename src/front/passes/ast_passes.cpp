#include "front/ast_passes.h"
#include "front/scope.h"
#include "front/error.h"

#include "AddImplicitCastsVisitor.h"

namespace lwnn { namespace front  { namespace passes {


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

        virtual bool visitingCompoundExpr(ast::CompoundExpr *compoundExpr) override {
            symbolTableStack_.push_back(compoundExpr->scope());
            return true;
        }

        virtual void visitedCompoundExpr(ast::CompoundExpr *compoundExpr) override {
            ASSERT(compoundExpr->scope() == symbolTableStack_.back());
            symbolTableStack_.pop_back();
        }

        virtual bool visitingCompoundStmt(ast::CompoundStmt *compoundStmt) override {
            symbolTableStack_.push_back(compoundStmt->scope());
            return true;
        }

        virtual void visitedCompoundStmt(ast::CompoundStmt *compoundStmt) override {
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

        virtual bool visitingCompoundStmt(ast::CompoundStmt *stmt) override {
            //The first entry on the stack would be the global scope which has no parent
            if(scopeDepth()) {
                stmt->scope()->setParent(topScope());
            }
            ScopeFollowingVisitor::visitingCompoundStmt(stmt);
            return true;
        }

        virtual bool visitingCompoundExpr(ast::CompoundExpr *expr) override {
            expr->scope()->setParent(topScope());
            ScopeFollowingVisitor::visitingCompoundExpr(expr);
            return true;
        }
    };


    class PopulateSymbolTablesAstVisitor : public ScopeFollowingVisitor {
        error::ErrorStream &errorStream_;
    public:

        PopulateSymbolTablesAstVisitor(error::ErrorStream &errorStream) : errorStream_(errorStream) {

        }

        virtual bool visitingClassDefinition(ast::ClassDefinition *cd) {
            scope::TypeSymbol *classSymbol = new scope::TypeSymbol(cd->classType());

            topScope()->addSymbol(classSymbol);

            return ScopeFollowingVisitor::visitingClassDefinition(cd);
        }

        virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
            if(topScope()->findSymbol(expr->name())) {
                errorStream_.error(expr->sourceSpan(), "Symbol '%s' is already defined in this scope.",
                                   expr->name().c_str());
            } else {
                auto symbol = new scope::VariableSymbol(expr->name());
                topScope()->addSymbol(symbol);
                expr->setSymbol(symbol);
                expr->typeRef()->addTypeResolutionListener(symbol);
            }
        }
    };


    class PopulateClassTypesAstVisitor : public ScopeFollowingVisitor {
        bool visitingClassDefinition(ast::ClassDefinition *cd) override {
            cd->populateClassType();
            return true;
        }
    };


    class SymbolResolvingAstVisitor : public ScopeFollowingVisitor {
        error::ErrorStream &errorStream_;
    public:
        SymbolResolvingAstVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

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
            //If it wasn't a primitive type...
            if(type == nullptr) {
                scope::Symbol* maybeType = topScope()->recursiveFindSymbol(typeRef->name());
                scope::TypeSymbol *classSymbol = dynamic_cast<scope::TypeSymbol*>(maybeType);

                if(classSymbol == nullptr) {
                    errorStream_.error(typeRef->sourceSpan(), "Symbol '%s' is not a type.", typeRef->name().c_str());
                    return;
                }

                type = classSymbol->type();
                typeRef->setType(type);
            }

            if(!type) {
                errorStream_.error(typeRef->sourceSpan(), "Undefined type '%s'.", typeRef->name().c_str());
            }
            typeRef->setType(type);
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
            }
        }
    };

    void runAllPasses(ast::Module *module, error::ErrorStream &es) {

        std::vector<ast::AstVisitor*> passes;
        passes.emplace_back(new SetSymbolTableParentsAstVisitor());
        passes.emplace_back(new PopulateSymbolTablesAstVisitor(es));
        passes.emplace_back(new SymbolResolvingAstVisitor(es));
        passes.emplace_back(new PopulateClassTypesAstVisitor());
        passes.emplace_back(new TypeResolvingVisitor(es));
        passes.emplace_back(new passes::AddImplicitCastsVisitor(es));
        passes.emplace_back(new BinaryExprSemanticsPass(es));
        passes.emplace_back(new CastExprSemanticPass(es));

        for(auto & pass : passes) {
            module->accept(pass);
            if(es.errorCount() > 0) {
                break;
            }
        }
    }
}}}