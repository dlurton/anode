#include "front/ast_passes.h"
#include "front/scope.h"
#include "front/error.h"

#include "AddImplicitCastsPass.h"

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
    virtual bool visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        symbolTableStack_.push_back(funcDeclStmt->parameterScope());
        return true;
    }

    virtual void visitedFuncDeclStmt(ast::FuncDefStmt *funcDeclStmt) override {
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
class SetSymbolTableParentsPass : public ScopeFollowingVisitor {
public:
    virtual bool visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        funcDeclStmt->parameterScope()->setParent(topScope());
        ScopeFollowingVisitor::visitingFuncDefStmt(funcDeclStmt);
        return true;
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


class PopulateSymbolTablesPass : public ScopeFollowingVisitor {
    error::ErrorStream &errorStream_;
public:

    PopulateSymbolTablesPass(error::ErrorStream &errorStream) : errorStream_(errorStream) {  }

    virtual bool visitingClassDefinition(ast::ClassDefinition *cd) override {
        scope::TypeSymbol *classSymbol = new scope::TypeSymbol(cd->classType());

        topScope()->addSymbol(classSymbol);

        return ScopeFollowingVisitor::visitingClassDefinition(cd);
    }

    virtual bool visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        scope::FunctionSymbol *funcSymbol = new scope::FunctionSymbol(funcDeclStmt->name(), funcDeclStmt->functionType());

        topScope()->addSymbol(funcSymbol);
        funcDeclStmt->setSymbol(funcSymbol);

        for(auto p : funcDeclStmt->parameters()) {
            scope::VariableSymbol *symbol = new scope::VariableSymbol(p->name(), p->type());
            if(funcDeclStmt->parameterScope()->findSymbol(p->name())) {
                errorStream_.error(
                    error::ErrorKind::SymbolAlreadyDefinedInScope,
                    p->span(),
                    "Duplicate parameter name '%s'",
                    p->name().c_str());
            } else {
                funcDeclStmt->parameterScope()->addSymbol(symbol);
                p->setSymbol(symbol);
            }
        }

        return ScopeFollowingVisitor::visitingFuncDefStmt(funcDeclStmt);
    }

    virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
        if(topScope()->findSymbol(expr->name())) {
            errorStream_.error(
                error::ErrorKind::SymbolAlreadyDefinedInScope,
                expr->sourceSpan(),
                "Symbol '%s' is already defined in this scope.",
                expr->name().c_str());

        } else {
            auto symbol = new scope::VariableSymbol(expr->name(), expr->typeRef()->type());
            topScope()->addSymbol(symbol);
            expr->setSymbol(symbol);
        }
    }
};


class PopulateClassTypesPass : public ScopeFollowingVisitor {
    bool visitingClassDefinition(ast::ClassDefinition *cd) override {
        cd->populateClassType();
        return true;
    }
};


class ResolveSymbolsPass : public ScopeFollowingVisitor {
    error::ErrorStream &errorStream_;
    gc_unordered_set<scope::Symbol*> definedSymbols_;
public:
    ResolveSymbolsPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    virtual void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
        if(expr->symbol() && expr->symbol()->storageKind() == scope::StorageKind::Local) {
            definedSymbols_.emplace(expr->symbol());
        }
    }

    virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) override {
        if(expr->symbol()) return;

        scope::Symbol *found = topScope()->recursiveFindSymbol(expr->name());
        if(!found) {
            errorStream_.error(error::ErrorKind::VariableNotDefined, expr->sourceSpan(),  "Symbol '%s' was not defined in this scope.",
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

class ResolveTypesPass : public ScopeFollowingVisitor {
    error::ErrorStream &errorStream_;
public:
    ResolveTypesPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) {

    }

    virtual void visitTypeRef(ast::TypeRef *typeRef) override {
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
        //Note:  we are not excluding implicit casts here...
        //This is a form of "double-checking" the AddImplicitCastsVisitor that we
        //get for free as long as we don't exclude implicit casts.

        type::Type *fromType = expr->valueExpr()->type();
        type::Type *toType = expr->type();

        if(fromType->canImplicitCastTo(toType)) return;

        if(expr->castKind() == ast::CastKind::Implicit) {
            ASSERT_FAIL("Implicit cast created for types that can't be implicitly cast.");
        }

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

    virtual void visitedDotExpr(ast::DotExpr *expr) override {
        if(!expr->lValue()->type()->isClass()) {
            errorStream_.error(
                error::ErrorKind::LeftOfDotNotClass,
                expr->dotSourceSpan(),
                "Type of value on left side of '.' operator is not an instance of a class.");
            return;
        }
        auto classType = static_cast<const type::ClassType*>(expr->lValue()->type()->actualType());
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

    virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) override {
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
    virtual void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) override {
        auto dotExpr = dynamic_cast<ast::DotExpr*>(binaryExpr->lValue());
        if(dotExpr && binaryExpr->operation() == ast::BinaryOperationKind::Assign) {
            dotExpr->setIsWrite(true);
        }
    }
};

class FuncCallSemanticsPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    FuncCallSemanticsPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    virtual void visitedFuncCallExpr(ast::FuncCallExpr *funcCallExpr) override {
        if(!funcCallExpr->funcExpr()->type()->isFunction()) {
            errorStream_.error(
                error::ErrorKind::OperatorCannotBeUsedWithType,
                funcCallExpr->sourceSpan(),
                "Result of expression left of '(' is not a function.");
            return;
        }

        type::FunctionType *funcType = dynamic_cast<type::FunctionType*>(funcCallExpr->funcExpr()->type());
        ASSERT(funcType);

        //When we do function overloading, this is going to get a whole lot more complicated.
        auto parameterTypes = funcType->parameterTypes();
        auto arguments = funcCallExpr->arguments();
        if(parameterTypes.size() != arguments.size()) {
            errorStream_.error(
                error::ErrorKind::IncorrectNumberOfArguments,
                funcCallExpr->sourceSpan(),
                "Incorrect number of arguments.  Expected %d but found %d",
                parameterTypes.size(),
                arguments.size());
        }

        for(size_t i = 0; i < arguments.size(); ++i) {
            auto argument = arguments[i];
            auto parameterType = parameterTypes[i];

            ASSERT(!parameterType->isClass() && "TODO: class instances as arguments");
            ASSERT(!parameterType->isClass() && "TODO: class instances as parameters");

            if(!parameterType->isSameType(argument->type())) {
                if(!argument->type()->canImplicitCastTo(parameterType)) {
                    errorStream_.error(
                        error::ErrorKind::InvalidImplicitCastInFunctionCallArgument,
                        argument->sourceSpan(),
                        "Cannot implicitly cast argument %d from '%s' to '%s'.",
                        i,
                        argument->type()->name().c_str(),
                        parameterType->name().c_str());
                } else {
                    auto implicitCast = ast::CastExpr::createImplicit(argument, parameterType);
                    funcCallExpr->replaceArgument(i, implicitCast);
                }
            }
        }
    }
};

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
    passes.emplace_back(new SetSymbolTableParentsPass());
    //Build the symbol tables so that symbol resolution works
    //Symbol tables are really just metadata generated from global definitions (i.e. class, func, etc.)
    passes.emplace_back(new PopulateSymbolTablesPass(es));
    //Resolve all ast::TypeRefs here (i.e. variables, arguments, class fields, function arguments, etc)
    //will know to the type::Type after this phase
    passes.emplace_back(new ResolveTypesPass(es));
    //Symbol references (i.e. variable, call sites, etc) find their corresponding symbols here.
    passes.emplace_back(new ResolveSymbolsPass(es));
    //Create type::ClassType and populate all the fields, for all classes
    passes.emplace_back(new PopulateClassTypesPass());
    //Resolve all member references
    passes.emplace_back(new ResolveDotExprMemberPass(es));
    //Insert implicit casts where they are allowed
    passes.emplace_back(new AddImplicitCastsPass(es));
    //Dot expressions immediately to the left of '=' should be properly marked as "writes" so the correct
    //LLVM IR can be emitted for them.  (No way to know this at parse time.)
    passes.emplace_back(new MarkDotExprWritesPass());

    //Finally, on to some semantics checking:

    passes.emplace_back(new BinaryExprSemanticsPass(es));
    passes.emplace_back(new CastExprSemanticPass(es));
    passes.emplace_back(new FuncCallSemanticsPass(es));

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