//
//
//#include "CompileAstVisitor.h"
//#include "common/containers.h"
//#include "front/ast.h"
//#include "emit.h"
//
//namespace lwnn { namespace back {
//
//class FuncDefSttmtAstVisitor : public CompileAstVisitor {
//
//public:
//    FuncDefSttmtAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}
//
//private:
//    virtual bool visitingFuncDeclStmt(ast::FuncDefStmt *) {
//        return true;
//    }
//
//    virtual void visitedFuncDeclStmt(ast::FuncDefStmt *) {
//
//    }
//};
//
//
//void emitFuncDef(lwnn::ast::FuncDefStmt *funcDef, CompileContext &cc) {
//    FuncDefSttmtAstVisitor visitor{cc};
//    funcDef->accept(&visitor);
//}
//
//}}
