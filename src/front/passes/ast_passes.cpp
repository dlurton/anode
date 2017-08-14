#include "front/ast_passes.h"
#include "front/scope.h"
#include "front/error.h"

#include "AddImplicitCastsVisitor.h"

namespace lwnn { namespace front  { namespace passes {

class ScopeFollowingVisitor : public ast::AstVisitor {

    //We use this only as a stack but it has to be a deque so we can iterate over its contents.
    gc_deque<scope::SymbolTable*> symbolTableStack_;
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

    PopulateSymbolTablesAstVisitor(error::ErrorStream &errorStream) : errorStream_(errorStream) {  }

    virtual bool visitingClassDefinition(ast::ClassDefinition *cd) {
        scope::TypeSymbol *classSymbol = new scope::TypeSymbol(cd->classType());

        topScope()->addSymbol(classSymbol);

        return ScopeFollowingVisitor::visitingClassDefinition(cd);
    }

    virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
        if(topScope()->findSymbol(expr->name())) {
            errorStream_.error(
                error::ErrorKind::SymbolAlreadyDefinedInScope,
                expr->sourceSpan(),
                "Symbol '%s' is already defined in this scope.",
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
    gc_unordered_set<scope::Symbol*> definedSymbols_;
public:
    SymbolResolvingAstVisitor(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) {
        if(expr->symbol() && expr->symbol()->storageKind() == scope::StorageKind::Local) {
            definedSymbols_.emplace(expr->symbol());
        }
    }

    virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) {
        if(expr->symbol()) return;

        scope::Symbol *found = topScope()->recursiveFindSymbol(expr->name());
        if(!found) {
            errorStream_.error(error::ErrorKind::VariableNotDefined, expr->sourceSpan(),  "Variable '%s' was not defined in this scope.",
                expr->name().c_str());
        } else {
            if(found->storageKind() == scope::StorageKind::Local && definedSymbols_.count(found) == 0) {
                errorStream_.error(
                    error::ErrorKind::VariableUsedBeforeDefinition, expr->sourceSpan(),  "Variable '%s' used before its definition.",
                    expr->name().c_str());
                return;
            }

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
                errorStream_.error(error::ErrorKind::SymbolIsNotAType, typeRef->sourceSpan(), "Symbol '%s' is not a type.", typeRef->name().c_str());
                return;
            }

            type = classSymbol->type();
            typeRef->setType(type);
        }

        if(!type) {
            errorStream_.error(error::ErrorKind::TypeNotDefined, typeRef->sourceSpan(), "Type '%s' was not defined in an accessible scope.", typeRef->name().c_str());
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
                error::ErrorKind::InvalidExplicitCast,
                expr->sourceSpan(),
                "Cannot cast from '%s' to '%s'",
                fromType->name().c_str(),
                toType->name().c_str());
        }
    }
};

class ResolveDotExprMemberPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    ResolveDotExprMemberPass(error::ErrorStream &errorStream) : errorStream_{errorStream} { }

    virtual void visitedDotExpr(ast::DotExpr *expr) {
        if(!expr->lValue()->type()->isClass()) {
            errorStream_.error(
                error::ErrorKind::LeftOfDotNotClass,
                expr->dotSourceSpan(),
                "Type of value on left side of '.' operator is not an instance of a class.");
            return;
        }
        auto classType = static_cast<type::ClassType*>(expr->lValue()->type());
        type::ClassField *field = classType->findField(expr->memberName());
        if(!field) {
            errorStream_.error(
                error::ErrorKind::ClassMemberNotFound,
                expr->dotSourceSpan(),
                "Class '%s' does not have a member named '%s'",
                classType->name().c_str(),
                expr->memberName().c_str());
            return;
        }
        expr->setField(field);
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
            if(!binaryExpr->lValue()->canWrite()) {
                errorStream_.error(
                    error::ErrorKind::CannotAssignToLValue,
                    binaryExpr->operatorSpan(), "Cannot assign a value to the expression left of '='");
            }
        } else if(binaryExpr->binaryExprKind() == ast::BinaryExprKind::Arithmetic) {
            if(!binaryExpr->type()->canDoArithmetic()) {
                errorStream_.error(
                    error::ErrorKind::OperatorCannotBeUsedWithType,
                    binaryExpr->operatorSpan(),
                    "Operator '%s' cannot be used with type '%s'.",
                    ast::to_string(binaryExpr->operation()).c_str(),
                    binaryExpr->type()->name().c_str());
            }
        }
    }
};

class MarkDotExprWritesPass : public ast::AstVisitor {
    virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) {
        auto dotExpr = dynamic_cast<ast::DotExpr*>(binaryExpr->lValue());
        if(dotExpr && binaryExpr->operation() == ast::BinaryOperationKind::Assign) {
            dotExpr->setIsWrite(true);
        }
    }
};
//
//class DotExprSemanticsPass : public ast::AstVisitor {
//    virtual void visitingDotExpr(ast::DotExpr *) {
//
//    }
//};

void runAllPasses(ast::Module *module, error::ErrorStream &es) {

    std::vector<ast::AstVisitor*> passes;

    //Having so many visitors is probably not great for performance because each of these visits the
    //entire tree but does very little in each individual pass. When/if it becomes an issue it should
    //be possible to merge some of these passes together. For now, the modularity of the existing
    //arrangement is highly desirable.

    // Order is important below.  There is necessary "temporal coupling"  and a requirement of
    // an at least partially mutable AST here but there's not an easy way around these as far as
    // I can tell because it's impossible to know all the information needed at parse time.

    //Symbol resolution works recursively, examining the current scope first and then
    //searching each parent until the symbol is found.
    passes.emplace_back(new SetSymbolTableParentsAstVisitor());
    //Build the symbol tables so that symbol resolution works
    //Symbol tables are really just metadata generated from global definitions (i.e. class, func, etc.)
    passes.emplace_back(new PopulateSymbolTablesAstVisitor(es));
    //Symbol references (i.e. variable, call sites, etc) find their corresponding symbols here.
    passes.emplace_back(new SymbolResolvingAstVisitor(es));
    //Create type::ClassType and populate all the fields, for all classes
    passes.emplace_back(new PopulateClassTypesAstVisitor());
    //Resolve all member references
    passes.emplace_back(new ResolveDotExprMemberPass(es));
    //Resolve all type references here (i.e. variables, arguments, class fields, function arguments, etc)
    //will know to the type::Type after this phase
    passes.emplace_back(new TypeResolvingVisitor(es));
    //Insert implicit casts where they are allowed
    passes.emplace_back(new AddImplicitCastsVisitor(es));
    //Dot expressions immediately to the left of '=' should be properly marked as "writes" so the correct
    //LLVM IR can be emitted for them.  (No way to know this at parse time.)
    passes.emplace_back(new MarkDotExprWritesPass());

    //Finally, on to some semantics checking:

    passes.emplace_back(new BinaryExprSemanticsPass(es));
    passes.emplace_back(new CastExprSemanticPass(es));

    for(ast::AstVisitor *pass : passes) {
        module->accept(pass);
        //If an error occurs during any pass, stop executing passes immediately because
        //each pass depends on the success of previous passes.
        if(es.errorCount() > 0) {
            break;
        }
    }
}

}}}