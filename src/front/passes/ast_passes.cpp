#include "front/ast_passes.h"

#include "ScopeFollowingAstVisitor.h"
#include "PopulateSymbolTablesPass.h"
#include "AddImplicitCastsPass.h"
#include "ResolveTypesPass.h"
#include "PopulateGenericTypesWithCompleteTypesPass.h"
#include "ConvertGenericTypeRefsToCompletePass.h"
#include "NamedTemplateExpanderPass.h"
#include "ResolveSymbolsPass.h"
#include "ResolveDotExprMemberPass.h"
#include "BinaryExprSemanticsPass.h"
#include "CastExprSemanticPass.h"
#include "FuncCallSemanticsPass.h"
#include "SetSymbolTableParentsPass.h"

#include "run_passes.h"
#include "ExpandGenericTypeReferencesPass.h"

namespace anode { namespace front  { namespace passes {

class PrepareClassesVisitor : public ast::AstVisitor {
    void visitingCompleteClassDefinition(ast::CompleteClassDefinition &cd) override {
        cd.populateClassType();
    }
};

class MarkDotExprWritesPass : public ast::AstVisitor {
    void visitedBinaryExpr(ast::BinaryExpr &binaryExpr) override {
        auto dotExpr = dynamic_cast<ast::DotExpr *>(&binaryExpr.lValue());
        if (dotExpr && binaryExpr.operation() == ast::BinaryOperationKind::Assign) {
            dotExpr->setIsWrite(true);
        }
    }
};

/** Stores the all templates in the AnodeWorld instance by UniqueId so they can be fetched later when they're expanded.
 * FIXME:  this class needs a better name. */
class TemplateWorldRecorderPass : public ScopeFollowingAstVisitor {
    ast::AnodeWorld &world_;
public:

    explicit TemplateWorldRecorderPass(error::ErrorStream &errorStream, ast::AnodeWorld &world)
        : ScopeFollowingAstVisitor(errorStream), world_(world) { }

    void visitingNamedTemplateExprStmt(ast::NamedTemplateExprStmt &templ) override {
        world_.addTemplate(templ);
        //body not normally visited
        templ.body().acceptVisitor(*this);
    }

    void visitingGenericClassDefinition(ast::GenericClassDefinition &genericClassDefinition) override {
        world_.addGenericClassDefinition(genericClassDefinition);
        //body not normally visited
        genericClassDefinition.body();
    }

    void visitingAnonymousTemplateExprStmt(ast::AnonymousTemplateExprStmt &anonymousTemplateExprStmt) override {
        //Body of anonymous template not normally visited...
        anonymousTemplateExprStmt.body().acceptVisitor(*this);
    }
};

bool runPasses(
    const gc_ref_vector<ast::AstVisitor> &visitors,
    ast::AstNode &node,
    error::ErrorStream &es,
    scope::SymbolTable *startingSymbolTable) {

    for(ast::AstVisitor &pass : visitors) {
        if(startingSymbolTable)
        if(auto sfav = dynamic_cast<ScopeFollowingAstVisitor*>(&pass)) {
            sfav->pushScope(*startingSymbolTable);
        }
        node.acceptVisitor(pass);
        //If an error occurs during any pass, stop executing passes immediately because
        //some passes depend on the success of previous passes.
        if(es.errorCount() > 0) {
            return true;
        }
    }
    return false;
}

gc_ref_vector<ast::AstVisitor> getPreTemplateExpansionPassses(ast::AnodeWorld &world, ast::Module &module, error::ErrorStream &es) {
    gc_ref_vector<ast::AstVisitor> passes;

    passes.emplace_back(*new TemplateWorldRecorderPass(es, world));

    //Symbol resolution works recursively, examining the current scope first and then
    //searching each parent until the symbol is found.
    passes.emplace_back(*new SetSymbolTableParentsPass(es, world));

    //Build the symbol tables so that symbol resolution works
    //Symbol tables are really just metadata generated from global definitions (i.e. class, func, etc.)
    passes.emplace_back(*new PopulateSymbolTablesPass(es));

    passes.emplace_back(*new NamedTemplateExpanderPass(es, module, world));

    return passes;
}

//TODO:  make this a method on AnodeWorld! Will need to move AnodeWorld out of ::ast first, however...
void runAllPasses(ast::AnodeWorld &world, ast::Module &module, error::ErrorStream &es) {

    //Having so many visitors is probably not great for performance because most of these visit the
    //entire tree but does very little in each individual pass. When/if performance becomes an issue it should
    //be possible to merge some of these passes together. For now, the modularity of the existing
    //arrangement is highly desirable.

    // Order of the individual passes is important because there is necessary "temporal coupling"  and a requirement of
    // an at least partially mutable AST here but there's not an easy way around these as far as
    // I can tell because it's impossible to know all the information needed at parse time.

    gc_ref_vector<ast::AstVisitor> passes;

    passes = getPreTemplateExpansionPassses(world, module, es);
    if(runPasses(passes, module, es)) return;

    passes.clear();

    //Resolve all ast::TypeRefs here (i.e. variables, arguments, class fields, function arguments, etc)
    //will know to the type::Type after this phase
    passes.emplace_back(*new ResolveTypesPass(es));

    passes.emplace_back(*new ExpandGenericTypeReferencesPass(es, module, world));

    passes.emplace_back(*new PopulateGenericTypesWithCompleteTypesPass(es));

    passes.emplace_back(*new ConvertGenericTypeRefsToCompletePass(es));

    //Symbol references (i.e. variable, call sites, etc) find their corresponding symbols here.
    passes.emplace_back(*new ResolveSymbolsPass(es));
    //Create type::ClassType and populate all the fields, for all classes
    passes.emplace_back(*new PrepareClassesVisitor());
    //Resolve all member references
    passes.emplace_back(*new ResolveDotExprMemberPass(es));
    //Insert implicit casts where they are allowed
    passes.emplace_back(*new AddImplicitCastsPass(es));
    //
    passes.emplace_back(*new BinaryExprSemanticsPass(es));

    //Finally, on to some semantics checking:
    passes.emplace_back(*new CastExprSemanticPass(es));
    passes.emplace_back(*new FuncCallSemanticsPass(es));

    //Dot expressions immediately to the left of '=' should be properly marked as "writes" so the correct
    //LLVM IR can be emitted for them.  (No way to know this at parse time.)
    passes.emplace_back(*new MarkDotExprWritesPass());

    runPasses(passes, module, es);
}

}}}