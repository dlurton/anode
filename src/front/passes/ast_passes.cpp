#include "front/ast_passes.h"
#include "front/scope.h"
#include "front/ErrorStream.h"

#include "AddImplicitCastsPass.h"

namespace anode { namespace front  { namespace passes {

bool runPasses(const gc_vector<ast::AstVisitor*> &visitors, ast::AstNode *node, error::ErrorStream &es, scope::SymbolTable *startingSymbolTable);
gc_vector<ast::AstVisitor*> getPreTemplateExpansionPassses(ast::AnodeWorld &world, error::ErrorStream &es);

class ErrorContextAstVisitor : public ast::AstVisitor {
protected:
    error::ErrorStream &errorStream_;

    ErrorContextAstVisitor(error::ErrorStream &errorStream) : errorStream_{errorStream} { }
public:

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        errorStream_.pushContextMessage("While inside template expansion at: " + expansion->sourceSpan().toString());
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *) override {
        errorStream_.popContextMessage();
    }
};

class ScopeFollowingAstVisitor : public ErrorContextAstVisitor {

    //We use this only as a stack but it has to be a deque so we can iterate over its contents.
    gc_deque<scope::SymbolTable*> symbolTableStack_;

protected:
    scope::SymbolTable *topScope() {
        ASSERT(symbolTableStack_.size());
        return symbolTableStack_.back();
    }

    size_t scopeDepth() { return symbolTableStack_.size(); }

    ScopeFollowingAstVisitor(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

public:
    void pushScope(scope::SymbolTable *st) {
        symbolTableStack_.push_back(st);
    }

    void visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        symbolTableStack_.push_back(funcDeclStmt->parameterScope());
    }

    void visitedFuncDeclStmt(ast::FuncDefStmt *funcDeclStmt) override {
        ASSERT(funcDeclStmt->parameterScope() == symbolTableStack_.back())
        symbolTableStack_.pop_back();
    }

    void visitingCompoundExpr(ast::CompoundExpr *compoundExpr) override {
        symbolTableStack_.push_back(compoundExpr->scope());
    }

    void visitedCompoundExpr(ast::CompoundExpr *compoundExpr) override {
        ASSERT(compoundExpr->scope() == symbolTableStack_.back());
        symbolTableStack_.pop_back();
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);
        symbolTableStack_.push_back(expansion->templateParameterScope());
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        ASSERT(expansion->templateParameterScope() == symbolTableStack_.back());
        symbolTableStack_.pop_back();
        ErrorContextAstVisitor::visitedTemplateExpansionExprStmt(expansion);
    }
};


/** Sets each SymbolTable's parent scope. */
class SetSymbolTableParentsPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
public:
    SetSymbolTableParentsPass(error::ErrorStream &errorStream, ast::AnodeWorld &world_)
        : ScopeFollowingAstVisitor(errorStream), world_(world_) { }

    void visitingModule(ast::Module *module) override {
        module->scope()->setParent(world_.globalScope());
    }
    void visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        funcDeclStmt->parameterScope()->setParent(topScope());
        ScopeFollowingAstVisitor::visitingFuncDefStmt(funcDeclStmt);
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {

        expansion->templateParameterScope()->setParent(topScope());
        ScopeFollowingAstVisitor::visitingTemplateExpansionExprStmt(expansion);
    }

    void visitingCompoundExpr(ast::CompoundExpr *expr) override {
        //The first entry on the stack would be the global scope which has no parent
        if(scopeDepth()) {
            expr->scope()->setParent(topScope());
        }
        ScopeFollowingAstVisitor::visitingCompoundExpr(expr);
    }
};

class PopulateSymbolTablesPass : public ScopeFollowingAstVisitor {
public:
    explicit PopulateSymbolTablesPass(error::ErrorStream &errorStream)
        : ScopeFollowingAstVisitor(errorStream) {  }

    scope::SymbolTable *currentScope() {
        scope::SymbolTable *ts = topScope();
        if (ts->storageKind() == scope::StorageKind::TemplateParameter) {
            return ts->parent();
        }

        return ts;
    }

//    void visitingGenericClassDefinition(ast::GenericClassDefinition *cd) override {
//
//    }

    void visitingCompleteClassDefinition(ast::CompleteClassDefinition *cd) override {
        //Classes that are defined within expanded templates do not get their own symbols
        //Only the generic version of them do. During symbol resolution, the symbol of the GenericType is
        //resolved and the resolved GenericType is used to determine the Type of the expanded class.
        if(!cd->hasTemplateArguments()) {
            type::Type *definedType = cd->definedType();

            if (auto definedClassType = dynamic_cast<type::ClassType *>(definedType)) {
                if (definedClassType->genericType() != nullptr) {
                    ScopeFollowingAstVisitor::visitingCompleteClassDefinition(cd);
                    return;
                }
            }

            auto *classSymbol = new scope::TypeSymbol(definedType);
            currentScope()->addSymbol(classSymbol);
        }

        ScopeFollowingAstVisitor::visitingCompleteClassDefinition(cd);
    }

    void visitingFuncDefStmt(ast::FuncDefStmt *funcDeclStmt) override {
        scope::FunctionSymbol *funcSymbol = new scope::FunctionSymbol(funcDeclStmt->name(), funcDeclStmt->functionType());

        currentScope()->addSymbol(funcSymbol);
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

        ScopeFollowingAstVisitor::visitingFuncDefStmt(funcDeclStmt);
    }

    void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
        if(currentScope()->findSymbol(expr->name())) {
            errorStream_.error(
                error::ErrorKind::SymbolAlreadyDefinedInScope,
                expr->sourceSpan(),
                "Symbol '%s' is already defined in this scope.",
                expr->name().c_str());

        } else {
            auto symbol = new scope::VariableSymbol(expr->name(), expr->typeRef()->type());
            currentScope()->addSymbol(symbol);
            expr->setSymbol(symbol);
        }
    }

    virtual void visitingTemplateExprStmt(ast::TemplateExprStmt *templ) override {
        if(currentScope()->findSymbol(templ->name())) {
            errorStream_.error(
                error::ErrorKind::SymbolAlreadyDefinedInScope,
                templ->sourceSpan(),
                "Symbol '%s' is already defined in this scope.",
                templ->name().c_str());
        } else {
            auto symbol = new scope::TemplateSymbol(templ->name(), templ->nodeId());
            currentScope()->addSymbol(symbol);
        }

        //Grab top-level classes within the scope of the template.
        for(auto exprStmt : templ->body()->expressions()) {
            if(auto cd = dynamic_cast<ast::GenericClassDefinition*>(exprStmt)) {
                if(currentScope()->findSymbol(cd->name())) {
                    errorStream_.error(
                        error::ErrorKind::SymbolAlreadyDefinedInScope,
                        templ->sourceSpan(),
                        "Symbol '%s' is already defined in this scope.",
                        templ->name().c_str());
                } else {
                    auto symbol = new scope::TypeSymbol(cd->name(), cd->definedType());
                    currentScope()->addSymbol(symbol);
                }
            }
        }
    }
};

class PrepareClassesVisitor : public ast::AstVisitor {
    void visitingCompleteClassDefinition(ast::CompleteClassDefinition *cd) override {
        cd->populateClassType();
    }
};

class ResolveSymbolsPass : public ScopeFollowingAstVisitor {
    gc_unordered_set<scope::Symbol*> definedSymbols_;
public:
    explicit ResolveSymbolsPass(error::ErrorStream &errorStream_) : ScopeFollowingAstVisitor(errorStream_) { }

    void visitingVariableDeclExpr(ast::VariableDeclExpr *expr) override {
        ASSERT(expr->symbol() && "Symbol must be resolved before this point.");
        if(expr->symbol()->storageKind() == scope::StorageKind::Local) {
            definedSymbols_.emplace(expr->symbol());
        }
    }

    void visitVariableRefExpr(ast::VariableRefExpr *expr) override {
        if(expr->symbol()) return;

        scope::Symbol *found = topScope()->recursiveFindSymbol(expr->name());
        if(!found) {
            errorStream_.error(error::ErrorKind::VariableNotDefined, expr->sourceSpan(),  "Symbol '%s' was not defined in this scope.",
                expr->name().c_str());
        } else {

            if(!found->type()->isClass() && !found->type()->isFunction()) {
                if (found->storageKind() == scope::StorageKind::Local && definedSymbols_.count(found) == 0) {
                    errorStream_.error(
                        error::ErrorKind::VariableUsedBeforeDefinition, expr->sourceSpan(), "Variable '%s' used before its definition.",
                        expr->name().c_str());
                    return;
                }
            }

            expr->setSymbol(found);
        }
    }
};

class ResolveTypesPass : public ScopeFollowingAstVisitor {
public:
    explicit ResolveTypesPass(error::ErrorStream &errorStream)
        : ScopeFollowingAstVisitor(errorStream) {
    }

    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef *typeRef) override {
        if(typeRef->isResolved()) {
            return;
        }
        std::string resolvedName = typeRef->name();
        type::Type* type = type::ScalarType::fromKeyword(resolvedName);

        //If it was a primitive type...
        if(type) {
            typeRef->setType(type);
            return;
        } else {
            scope::Symbol* maybeType = topScope()->recursiveFindSymbol(typeRef->name());

            //Symbol doesn't exist in accessible scope?
            if(!maybeType) {
                errorStream_.error(error::ErrorKind::TypeNotDefined, typeRef->sourceSpan(), "Type '%s' was not defined in an accessible scope.", typeRef->name().c_str());
                return;
            }

            auto *typeSymbol = dynamic_cast<scope::TypeSymbol*>(maybeType);

            //Symbol does exist but isn't a type.
            if(typeSymbol == nullptr) {
                errorStream_.error(error::ErrorKind::SymbolIsNotAType, typeRef->sourceSpan(), "Symbol '%s' is not a type.", typeRef->name().c_str());
                return;
            }

            type = typeSymbol->type();
            typeRef->setType(type);
        }
    }
};


class CastExprSemanticPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit CastExprSemanticPass(error::ErrorStream &errorStream_) : errorStream_(errorStream_) { }

    void visitingCastExpr(ast::CastExpr *expr) override {
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
                fromType->nameForDisplay().c_str(),
                toType->nameForDisplay().c_str());
        }
    }
};

class ResolveDotExprMemberPass : public ast::AstVisitor {
    error::ErrorStream &errorStream_;
public:
    explicit ResolveDotExprMemberPass(error::ErrorStream &errorStream) : errorStream_{errorStream} { }

    void visitedDotExpr(ast::DotExpr *expr) override {
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
                classType->nameForDisplay().c_str(),
                expr->memberName().c_str());
            return;
        }
        expr->setField(field);
    }

    void visitedFuncCallExpr(ast::FuncCallExpr *expr) override {
        auto methodRef = dynamic_cast<ast::MethodRefExpr*>(expr->funcExpr());
        if(methodRef && expr->instanceExpr()) {
            type::Type *instanceType = expr->instanceExpr()->type()->actualType();
            type::ClassMethod *method = nullptr;

            if(auto classType = dynamic_cast<type::ClassType*>(instanceType)) {
                method = classType->findMethod(methodRef->name());
            }

            if(method) {
                methodRef->setSymbol(method->symbol());
            } else {
                errorStream_.error(
                    error::ErrorKind::MethodNotDefined,
                    methodRef->sourceSpan(),
                    "Type '%s' does not have a method named '%s'.",
                    instanceType->nameForDisplay().c_str(),
                    methodRef->name().c_str());
            }
        }
    }
};

class BinaryExprSemanticsPass : public ErrorContextAstVisitor {
public:
    explicit BinaryExprSemanticsPass(error::ErrorStream &errorStream_) : ErrorContextAstVisitor(errorStream_) { }

    void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) override {
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
                    binaryExpr->type()->nameForDisplay().c_str());
            }
        }
    }
};

class MarkDotExprWritesPass : public ast::AstVisitor {
    void visitedBinaryExpr(ast::BinaryExpr *binaryExpr) override {
        auto dotExpr = dynamic_cast<ast::DotExpr *>(binaryExpr->lValue());
        if (dotExpr && binaryExpr->operation() == ast::BinaryOperationKind::Assign) {
            dotExpr->setIsWrite(true);
        }
    }
};

class FuncCallSemanticsPass : public ErrorContextAstVisitor {
public:
    explicit FuncCallSemanticsPass(error::ErrorStream &errorStream_) : ErrorContextAstVisitor(errorStream_) { }

    void visitedFuncCallExpr(ast::FuncCallExpr *funcCallExpr) override {
        if(!funcCallExpr->funcExpr()->type()->isFunction()) {
            errorStream_.error(
                error::ErrorKind::OperatorCannotBeUsedWithType,
                funcCallExpr->sourceSpan(),
                "Result of expression left of '(' is not a function.");
            return;
        }

        auto *funcType = dynamic_cast<type::FunctionType*>(funcCallExpr->funcExpr()->type());
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
            return;
        }

        for(size_t i = 0; i < arguments.size(); ++i) {
            auto argument = arguments[i];
            auto parameterType = parameterTypes[i];

            if(!parameterType->isSameType(argument->type())) {
                if(!argument->type()->canImplicitCastTo(parameterType)) {
                    errorStream_.error(
                        error::ErrorKind::InvalidImplicitCastInFunctionCallArgument,
                        argument->sourceSpan(),
                        "Cannot implicitly cast argument %d from '%s' to '%s'.",
                        i,
                        argument->type()->nameForDisplay().c_str(),
                        parameterType->nameForDisplay().c_str());
                } else {
                    auto implicitCast = ast::CastExpr::createImplicit(argument, parameterType);
                    funcCallExpr->replaceArgument(i, implicitCast);
                }
            }
        }
    }
};

/** Stores the all templates in the AnodeWorld instance by UniqueId so they can be fetched later when they're expanded. */
class TemplateWorldRecorderPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
public:
    explicit TemplateWorldRecorderPass(error::ErrorStream &errorStream, ast::AnodeWorld &world)
        : ScopeFollowingAstVisitor(errorStream), world_(world) { }

    virtual void visitingTemplateExprStmt(ast::TemplateExprStmt *templ) override {
        world_.addTemplate(templ);
    }
};

/** Enacts explicit template expansions. */
class TemplateExpanderPass : public ErrorContextAstVisitor {
    ast::AnodeWorld &world_;
    ast::Module *module_ = nullptr;
    bool visitingExpansion_ = false;

public:

    TemplateExpanderPass(error::ErrorStream &errorStream, ast::AnodeWorld &world_)
        : ErrorContextAstVisitor(errorStream), world_(world_) { }

    bool shouldVisitChildren() override {
        return !visitingExpansion_;
    }


    void visitingModule(ast::Module *module) override {
        module_ = module;
    }

    void visitedTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        visitingExpansion_ = false;
        ErrorContextAstVisitor::visitedTemplateExpansionExprStmt(expansion);

    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);
        visitingExpansion_ = true;
        if(expansion->expandedTemplate() != nullptr) {
            return;
        }
        scope::Symbol *foundSymbol = expansion->templateParameterScope()->parent()->recursiveFindSymbol(expansion->templatedId().text());
        if(!foundSymbol) {
            errorStream_.error(
                error::ErrorKind::TemplateDoesNotExist,
                expansion->templatedId().span(),
                "Temolate does not exist within the current scope");
            return;
        }

        auto templateSymbol = upcast<scope::TemplateSymbol>(foundSymbol);
        ast::TemplateExprStmt *templ = world_.getTemplate(templateSymbol->astNodeId());
        ASSERT(templ && "Couldn't find template by astNodeId");

        //For each template argument
        //Create create a Symbol of name of the corresponding parameter referring to the TypeRef.  (AliasSymbol?)
        gc_vector<ast::TemplateParameter*> tParams = templ->parameters();
        gc_vector<ast::TypeRef*> tArgs = expansion->typeArguments();

        if(tParams.size() != tArgs.size()) {
            const char *wasWere = tArgs.size() == 1 ? "was" : "were";
            errorStream_.error(
                error::ErrorKind::IncorrectNumberOfTemplateArguments,
                expansion->sourceSpan(),
                "Incorrect number of template arguments - expected %d but %d %s specified",
                tParams.size(),
                tArgs.size(),
                wasWere);
            return;
        }

        //TODO:  refactor TemplateExpansionExprStmt to store this as a field.
        gc_vector<ast::TemplateArgument*> templateArgs;

        for(unsigned int i = 0; i < tParams.size(); ++i) {
            expansion->templateParameterScope()->addSymbol(new scope::TypeSymbol(tParams[i]->name(), tArgs[i]->type()));
            templateArgs.push_back(new ast::TemplateArgument(tParams[i]->name(), tArgs[i]));
        }

        expansion->setExpandedTemplate(templ->body()->deepCopyExpandTemplate(templateArgs));

        auto visitors = getPreTemplateExpansionPassses(world_, errorStream_);
        runPasses(visitors, expansion->expandedTemplate(), errorStream_, expansion->templateParameterScope());
    }
};

/**
 * Adds each CompleteClassDefinition instance and the template arguments used to expand it to the GenericClassDefinition
 * from which it originated so that it may later be resolved.
 */
class PopulateGenericTypesWithCompleteTypesPass : public ErrorContextAstVisitor {

    class PopulateGenericTypesSubPass : public ScopeFollowingAstVisitor {
        gc_vector<type::Type*> templateArguments_;
    public:
        PopulateGenericTypesSubPass(error::ErrorStream &errorStream, const gc_vector<type::Type *> &templateArgs)
            : ScopeFollowingAstVisitor(errorStream), templateArguments_(templateArgs) { }
        void visitingCompleteClassDefinition(ast::CompleteClassDefinition *cd) override {
            auto genericType = upcast<type::ClassType>(cd->definedType())->genericType();
            ASSERT(genericType);
            if(genericType->findExpandedClassType(templateArguments_)) {
                return;
            }
            genericType->addExpandedClass(templateArguments_, upcast<type::ClassType>(cd->definedType()));
        }
    };

public:
    explicit PopulateGenericTypesWithCompleteTypesPass(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt *expansion) override {
        ErrorContextAstVisitor::visitingTemplateExpansionExprStmt(expansion);

        gc_vector<type::Type*> typeArguments;
        gc_vector<scope::TypeSymbol*> argumentSymbols = expansion->templateParameterScope()->types();
        typeArguments.reserve(argumentSymbols.size());
        for(auto argSymbol : argumentSymbols) {
            typeArguments.push_back(argSymbol->type());
        }

        PopulateGenericTypesSubPass pass{errorStream_, typeArguments};
        pass.pushScope(expansion->templateParameterScope());
        expansion->expandedTemplate()->accept(&pass);
    }
};

/**
 * After symbol table resolution reference to generic types have been resolved to GenericType instances.
 * This this visitor will look up the type::ClassType which was added to the type::GenericType instances during
 * PopulateGenericTypesWithCompleteTypesPass.
 */
class ConvertGenericTypeRefsToCompletePass : public ErrorContextAstVisitor {
public:
    ConvertGenericTypeRefsToCompletePass(error::ErrorStream &errorStream) : ErrorContextAstVisitor(errorStream) { }

    void visitedResolutionDeferredTypeRef(ast::ResolutionDeferredTypeRef *typeRef) override {
        if(typeRef->type()->isGeneric()) {
            gc_vector<type::Type*> templateArgs = typeRef->resolutionDeferredType()->typeArguments();
            auto genericType = upcast<type::GenericType>(typeRef->type()->actualType());
            if(genericType->templateParameterCount() != (int)templateArgs.size()) {
                errorStream_.error(
                    error::ErrorKind::IncorrectNumberOfGenericArguments,
                    typeRef->sourceSpan(),
                    "Incorrect number of generic arguments for type '%s' - expected %d but found %d",
                    genericType->nameForDisplay().c_str(),
                    genericType->templateParameterCount(),
                    templateArgs.size());
                return;
            }

            type::ClassType *expandedType = genericType->findExpandedClassType(templateArgs);
            if(!expandedType) {
                errorStream_.error(
                    error::ErrorKind::GenericTypeWasNotExpandedWithSpecifiedArguments,
                    typeRef->sourceSpan(),
                    "Generic type '%s' was not expanded with the specified type arguments",
                    genericType->nameForDisplay().c_str());
                return;
            }

            typeRef->setType(expandedType);

        } else if(typeRef->hasTemplateArguments()) {
            errorStream_.error(
                error::ErrorKind::TypeIsNotGenericButIsReferencedWithGenericArgs,
                typeRef->sourceSpan(),
                "Type '%s' is not generic but is referenced with generic arguments",
                typeRef->type()->nameForDisplay().c_str());
        }
    }
};

bool runPasses(
    const gc_vector<ast::AstVisitor*> &visitors,
    ast::AstNode *node,
    error::ErrorStream &es,
    scope::SymbolTable *startingSymbolTable) {

    for(ast::AstVisitor *pass : visitors) {
        if(startingSymbolTable)
        if(auto sfav = dynamic_cast<ScopeFollowingAstVisitor*>(pass)) {
            sfav->pushScope(startingSymbolTable);
        }
        node->accept(pass);
        //If an error occurs during any pass, stop executing passes immediately because
        //some passes depend on the success of previous passes.
        if(es.errorCount() > 0) {
            return true;
        }
    }
    return false;
}


gc_vector<ast::AstVisitor*> getPreTemplateExpansionPassses(ast::AnodeWorld &world, error::ErrorStream &es) {
    gc_vector<ast::AstVisitor*> passes;

    //Symbol resolution works recursively, examining the current scope first and then
    //searching each parent until the symbol is found.
    passes.push_back(new SetSymbolTableParentsPass(es, world));

    //Build the symbol tables so that symbol resolution works
    //Symbol tables are really just metadata generated from global definitions (i.e. class, func, etc.)
    passes.push_back(new PopulateSymbolTablesPass(es));

    passes.push_back(new TemplateExpanderPass(es, world));

    return passes;
}

//TODO:  make this a method on AnodeWorld! Will need to move AnodeWorld out of ::ast first, however...
void runAllPasses(ast::AnodeWorld &world, ast::Module *module, error::ErrorStream &es) {

    //Having so many visitors is probably not great for performance because most of these visit the
    //entire tree but does very little in each individual pass. When/if performance becomes an issue it should
    //be possible to merge some of these passes together. For now, the modularity of the existing
    //arrangement is highly desirable.

    // Order of the individual passes is important because there is necessary "temporal coupling"  and a requirement of
    // an at least partially mutable AST here but there's not an easy way around these as far as
    // I can tell because it's impossible to know all the information needed at parse time.

    gc_vector<ast::AstVisitor*> passes;
    passes.push_back(new TemplateWorldRecorderPass(es, world));
    if(runPasses(passes, module, es, nullptr)) return;

    passes = getPreTemplateExpansionPassses(world, es);
    if(runPasses(passes, module, es, nullptr)) return;

    passes.clear();

    //Resolve all ast::TypeRefs here (i.e. variables, arguments, class fields, function arguments, etc)
    //will know to the type::Type after this phase
    passes.push_back(new ResolveTypesPass(es));

    passes.push_back(new PopulateGenericTypesWithCompleteTypesPass(es));

    passes.push_back(new ConvertGenericTypeRefsToCompletePass(es));

    //Symbol references (i.e. variable, call sites, etc) find their corresponding symbols here.
    passes.emplace_back(new ResolveSymbolsPass(es));
    //Create type::ClassType and populate all the fields, for all classes
    passes.emplace_back(new PrepareClassesVisitor());
    //Resolve all member references
    passes.emplace_back(new ResolveDotExprMemberPass(es));
    //Insert implicit casts where they are allowed
    passes.emplace_back(new AddImplicitCastsPass(es));
    //
    passes.emplace_back(new BinaryExprSemanticsPass(es));

    //Finally, on to some semantics checking:
    passes.emplace_back(new CastExprSemanticPass(es));
    passes.emplace_back(new FuncCallSemanticsPass(es));

    //Dot expressions immediately to the left of '=' should be properly marked as "writes" so the correct
    //LLVM IR can be emitted for them.  (No way to know this at parse time.)
    passes.emplace_back(new MarkDotExprWritesPass());

    runPasses(passes, module, es, nullptr);
}

}}}