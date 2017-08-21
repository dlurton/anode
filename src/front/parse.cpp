#include "front/ast.h"
#include "front/error.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <antlr4-runtime.h>
#pragma GCC diagnostic pop

#include "generated/LwnnLexer.h"
#include "generated/LwnnParser.h"
#include "generated/LwnnBaseListener.h"

using namespace lwnn::ast;
using namespace lwnn_parser;
using namespace antlr4;

namespace lwnn { namespace parse {
/** This namespace contains types and functions mainly associated with invoking the generated
 * ANTLR4 parser and converting parse tree to the LWNN AST.
 */

ast::ExprStmt *extractExpr(LwnnParser::ExprContext *ctx);
ast::ExprStmt *extractExprStmt(LwnnParser::ExprStmtContext *ctx);
ast::CompoundExpr *extractCompoundExpr(LwnnParser::CompoundExprStmtContext *ctx);

/** Extracts a SourceRange from values specified in token. */
static source::SourceSpan getSourceSpan(antlr4::Token *token) {
    auto startSource = token->getTokenSource();

    //NOTE:  this is kinda not so good because it assumes tokens never span lines, which is fine for most
    //tokens since they don't span lines.  But if I ever decide to implement multi-line strings...
    return source::SourceSpan(startSource->getSourceName(),
          source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
          source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine() + token->getText().length()));
}

/** Extracts a SourceRange from values specified in ctx. */
static source::SourceSpan getSourceSpan(antlr4::ParserRuleContext *ctx) {
    auto startSource = ctx->getStart()->getTokenSource();
    auto endSource = ctx->getStop()->getTokenSource();

    return source::SourceSpan(startSource->getSourceName(),
              source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
              source::SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
}

/** Extracts a SourceRange from values specified in ctx. */
static source::SourceSpan getSourceSpan(antlr4::ParserRuleContext *startContext, antlr4::ParserRuleContext *endContext) {
    auto startSource = startContext->getStart()->getTokenSource();
    auto endSource = endContext->getStop()->getTokenSource();

    return source::SourceSpan(startSource->getSourceName(),
              source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
              source::SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
}

ast::TypeRef *extractTypeRef(LwnnParser::TypeRefContext *ctx) {
    return new TypeRef(getSourceSpan(ctx), ctx->getText());
}

/*******************************************/
template<typename TAstNode>
class LwnnBaseListenerHelper : public LwnnBaseListener {
    TAstNode *resultNode_ = nullptr;
protected:
    void setResult(TAstNode *node) {
        resultNode_ = node;
    }
public:
    bool hasResult() { return resultNode_ != nullptr; }

    TAstNode *surrenderResult() {
        ASSERT(resultNode_ != nullptr && "Result has not been set");
        return resultNode_;
    }
};


/*******************************************/
class ExprListener : public LwnnBaseListenerHelper<ast::ExprStmt> {
public:
    virtual void enterParensExpr(LwnnParser::ParensExprContext *ctx) override {
        ExprListener listener;
        ctx->expr()->enterRule(&listener);
        if(!listener.hasResult()) return;
        setResult(listener.surrenderResult());
    }

    virtual void enterVarDeclExpr(LwnnParser::VarDeclExprContext *ctx) override {
        auto typeRef = extractTypeRef(ctx->typeRef());

        setResult(new ast::VariableDeclExpr(getSourceSpan(ctx), ctx->name->getText(), typeRef));
    }

    virtual void enterCastExpr(LwnnParser::CastExprContext *ctx) override {
        ExprListener listener;
        ctx->expr()->enterRule(&listener);
        if(!listener.hasResult()) return;

        source::SourceSpan sourceSpan = getSourceSpan(ctx);
        auto typeRef = extractTypeRef(ctx->type);

        auto castExpr = new CastExpr(
            getSourceSpan(ctx),
            typeRef,
            listener.surrenderResult(),
            CastKind::Explicit);

        setResult(castExpr);
    }

    virtual void enterLiteralBool(LwnnParser::LiteralBoolContext *ctx) override {
        if(ctx->getText() == "true") {
            setResult(new LiteralBoolExpr(getSourceSpan(ctx), true));
        } else if(ctx->getText() == "false") {
            setResult(new LiteralBoolExpr(getSourceSpan(ctx), false));
        } else {
            ASSERT_FAIL("Parser parsed something that it thinks is a boolean literal but wasn't 'true' or 'false'.");
        }
    }

    virtual void enterLiteralInt32Expr(LwnnParser::LiteralInt32ExprContext *ctx) override {
        int value = std::stoi(ctx->getText());
        setResult(new LiteralInt32Expr(getSourceSpan(ctx), value));
    }

    virtual void enterLiteralFloatExpr(LwnnParser::LiteralFloatExprContext *ctx) override {
        double value = std::stof(ctx->getText());
        setResult(new LiteralFloatExpr(getSourceSpan(ctx), value));
    }

    virtual void enterBinaryExpr(LwnnParser::BinaryExprContext *ctx) override {
        ExprListener leftListener;
        ctx->left->enterRule(&leftListener);
        if(!leftListener.hasResult()) return;

        ExprListener rightListener;
        ctx->right->enterRule(&rightListener);
        if(!rightListener.hasResult()) return;

        BinaryOperationKind opKind;
        ExprStmt *leftExpr = leftListener.surrenderResult();

        switch(ctx->op->getType()) {
            case LwnnParser::OP_ASSIGN: {
                opKind = BinaryOperationKind::Assign;
                ast::VariableRefExpr *varRef = dynamic_cast<ast::VariableRefExpr*>(leftExpr);
                if(varRef != nullptr) {
                    varRef->setVariableAccess(VariableAccess::Write);
                }
                break;
            }
            case LwnnParser::OP_ADD:    opKind = BinaryOperationKind::Add; break;
            case LwnnParser::OP_SUB:    opKind = BinaryOperationKind::Sub; break;
            case LwnnParser::OP_MUL:    opKind = BinaryOperationKind::Mul; break;
            case LwnnParser::OP_DIV:    opKind = BinaryOperationKind::Div; break;
            case LwnnParser::OP_EQ:     opKind = BinaryOperationKind::Eq; break;
            case LwnnParser::OP_NEQ:    opKind = BinaryOperationKind::NotEq; break;
            case LwnnParser::OP_AND:    opKind = BinaryOperationKind::LogicalAnd; break;
            case LwnnParser::OP_OR:     opKind = BinaryOperationKind::LogicalOr; break;
            case LwnnParser::OP_GT:     opKind = BinaryOperationKind::GreaterThan; break;
            case LwnnParser::OP_LT:     opKind = BinaryOperationKind::LessThan; break;
            case LwnnParser::OP_GTE:    opKind = BinaryOperationKind::GreaterThanOrEqual; break;
            case LwnnParser::OP_LTE:    opKind = BinaryOperationKind::LessThanOrEqual; break;

            default:
                ASSERT_FAIL("Unhandled Token Type (Operators)");
        }


        ExprStmt *rightExpr = rightListener.surrenderResult();
        auto node = new BinaryExpr(
                            getSourceSpan(ctx->left, ctx->right),
                            leftExpr,
                            opKind,
                            getSourceSpan(ctx->op),
                            rightExpr);
        setResult(node);
    }

    virtual void enterDotExpr(LwnnParser::DotExprContext *ctx) override {
        setResult(
            new DotExpr(
                getSourceSpan(ctx),
                getSourceSpan(ctx->op),
                extractExpr(ctx->left),
                ctx->right->getText()));
    }

    virtual void enterVariableRefExpr(LwnnParser::VariableRefExprContext *ctx) override {
        setResult(new VariableRefExpr(getSourceSpan(ctx), ctx->getText()));
    }

    virtual void enterFuncCallExpr(LwnnParser::FuncCallExprContext * ctx) override {
        LwnnParser::ExprListContext *current = ctx->exprList();
        gc_vector<ExprStmt*> arguments;
        while(current) {
            arguments.push_back(extractExpr(current->expr()));
            current = current->exprList();
        }

        setResult(new FuncCallExpr(
            getSourceSpan(ctx),
            getSourceSpan(ctx->op),
            extractExpr(ctx->expr()),
            arguments));
    }

    virtual void enterTernaryExpr(LwnnParser::TernaryExprContext *ctx) override {
        if(!ctx->cond) return;
        ExprListener condListener;
        ctx->cond->enterRule(&condListener);
        if(!condListener.hasResult()) return;

        if(!ctx->thenExpr) return;
        ExprListener thenListener;
        ctx->thenExpr->enterRule(&thenListener);
        if(!thenListener.hasResult()) return;

        ExprStmt *elseExpr;

        if(ctx->elseExpr) {
            ExprListener elseListener;
            ctx->elseExpr->enterRule(&elseListener);
            if (!thenListener.hasResult()) return;
            elseExpr = elseListener.surrenderResult();
        }

        setResult(
            new IfExprStmt(getSourceSpan(ctx),
                                     condListener.surrenderResult(),
                                     thenListener.surrenderResult(),
                                     elseExpr));
    }

    virtual void enterIfExpr(LwnnParser::IfExprContext *ctx) override {
        ASSERT(ctx->cond);
        ExprListener condListener;
        ctx->cond->enterRule(&condListener);
        if(!condListener.hasResult()) return;

        ast::ExprStmt *thenExprStmt = extractExpr(ctx->thenExpr);
        ast::ExprStmt *elseExprStmt = extractExpr(ctx->elseExpr);

        setResult(
            new IfExprStmt(
                getSourceSpan(ctx),
                condListener.surrenderResult(),
                thenExprStmt,
                elseExprStmt));
    }

    virtual void enterCompoundExpr(LwnnParser::CompoundExprContext * ctx) override {
        setResult(extractCompoundExpr(ctx->compoundExprStmt()));
    }
};

ast::ExprStmt *extractExpr(LwnnParser::ExprContext *ctx) {
    if(!ctx) return nullptr;
    ExprListener listener;
    ctx->enterRule(&listener);
    return listener.hasResult() ? listener.surrenderResult() : nullptr;
}

/*******************************************/
class ExprStmtListener : public LwnnBaseListenerHelper<ast::ExprStmt> {
public:

    virtual void enterSimpleExpr(LwnnParser::SimpleExprContext *ctx) override {
        ExprListener listener;
        ctx->expr()->enterRule(&listener);
        if(!listener.hasResult()) return;
        setResult(listener.surrenderResult());
    }
    virtual void enterIfStmt(LwnnParser::IfStmtContext *ctx) override {
        ASSERT(ctx->cond);
        ExprListener condListener;
        ctx->cond->enterRule(&condListener);
        if(!condListener.hasResult()) return;

        ast::ExprStmt *thenExprStmt = extractExprStmt(ctx->thenStmt);
        ast::ExprStmt *elseExprStmt = extractExprStmt(ctx->elseStmt);

        setResult(
            new IfExprStmt(getSourceSpan(ctx),
                                         condListener.surrenderResult(),
                                         thenExprStmt,
                                         elseExprStmt));
    }

    virtual void enterWhileStmt(LwnnParser::WhileStmtContext *ctx) override {
        ASSERT(ctx->cond);
        ASSERT(ctx->body);
        ExprStmt *condition = extractExpr(ctx->cond);
        ExprStmt *body = extractExpr(ctx->body);

        setResult(
            new WhileExpr(
                getSourceSpan(ctx),
                condition,
                body
            )
        );
    }

    virtual void enterWhileStmtCompound(LwnnParser::WhileStmtCompoundContext *ctx) override {
        ASSERT(ctx->cond);
        ASSERT(ctx->body);
        ExprStmt *condition = extractExpr(ctx->cond);
        ExprStmt *body = extractCompoundExpr(ctx->body);

        setResult(
            new WhileExpr(
                getSourceSpan(ctx),
                condition,
                body
            )
        );
    }

    virtual void enterCompoundStmt(LwnnParser::CompoundStmtContext *ctx) override {
        setResult(extractCompoundExpr(ctx->compoundExprStmt()));
    }

};

ast::ExprStmt *extractExprStmt(LwnnParser::ExprStmtContext *ctx) {
    if(!ctx) return nullptr;

    ExprStmtListener listener;
    ctx->enterRule(&listener);
    return listener.hasResult() ? listener.surrenderResult() : nullptr;
}

/*******************************************/
class CompoundExprListener : public LwnnBaseListenerHelper<ast::CompoundExpr> {
public:
    virtual void enterCompoundExprStmt(LwnnParser::CompoundExprStmtContext * ctx) override {
        std::vector<LwnnParser::ExprStmtContext*> expressions = ctx->exprStmt();
        auto compoundExpr = new ast::CompoundExpr(getSourceSpan(ctx));
        for (LwnnParser::ExprStmtContext *expr : expressions) {
            ExprStmt *exprStmt = extractExprStmt(expr);
            if(exprStmt) {
                compoundExpr->addExpr(exprStmt);
            }
        }
        setResult(compoundExpr);
    }
};

ast::CompoundExpr *extractCompoundExpr(LwnnParser::CompoundExprStmtContext *ctx) {
    if(!ctx) return nullptr;
    CompoundExprListener listener;
    ctx->enterRule(&listener);
    return listener.hasResult() ? listener.surrenderResult() : nullptr;
}

class StmtListener : public LwnnBaseListenerHelper<ast::Stmt> {
public:
    virtual void enterExpressionStatement(LwnnParser::ExpressionStatementContext *ctx) override {
        setResult(extractExprStmt(ctx->exprStmt()));
    }

    virtual void enterFunctionDefinition(LwnnParser::FunctionDefinitionContext *ctx) override {
        if(!ctx->funcDef()) return;

        LwnnParser::FuncDefContext *funcDef = ctx->funcDef();

        //Get TypeRef
        LwnnParser::TypeRefContext *typeRefCtx = funcDef->typeRef();
        if(!typeRefCtx) return;
        ast::TypeRef *typeRef = extractTypeRef(typeRefCtx);

        //Get Parameters
        gc_vector<ast::ParameterDef*> parameters;
        LwnnParser::ParameterDefListContext* currentParameterDefListCtx = funcDef->parameterDefList();
        while(currentParameterDefListCtx != nullptr) {
            auto parameterDefCtx = currentParameterDefListCtx->parameterDef();
            auto parameterDef = new ast::ParameterDef(
                getSourceSpan(parameterDefCtx),
                parameterDefCtx->name->getText(),
                extractTypeRef((parameterDefCtx->typeRef()))
            );
            parameters.push_back(parameterDef);
            currentParameterDefListCtx = currentParameterDefListCtx->parameterDefList();
        }

        //Get body
        ExprStmt *funcBody = extractExprStmt(funcDef->body);

        source::SourceSpan span = getSourceSpan(funcDef);
        std::string name = funcDef->name->getText();
        setResult(new FuncDefStmt(span, name, typeRef, parameters, funcBody));
    }

    virtual void enterClassDefinition(LwnnParser::ClassDefinitionContext *ctx) override {
        if(!ctx->classDef()) return;
        if(!ctx->classDef()->classBody()) return;

        CompoundStmt *compoundStmt = new CompoundStmt(getSourceSpan(ctx), scope::StorageKind::Instance);
        for (LwnnParser::StmtContext *stmt : ctx->classDef()->classBody()->stmt()) {
            if(stmt == nullptr) continue;
            StmtListener listener;
            stmt->enterRule(&listener);
            if (!listener.hasResult()) continue;
            compoundStmt->addStmt(listener.surrenderResult());
        }

        auto classDef = new ast::ClassDefinition(
            getSourceSpan(ctx), ctx->classDef()->name->getText(), compoundStmt);

        setResult(classDef);
    }
};

/*******************************************/
class ModuleListener : public LwnnBaseListenerHelper<ast::Module> {
    std::string moduleName_;

public:
    ModuleListener(const std::string &moduleName_) : moduleName_(moduleName_) { }

    virtual void enterModule(LwnnParser::ModuleContext *ctx) override {
        std::vector<LwnnParser::StmtContext*> statements = ctx->stmt();
        auto compoundStmt = new ast::CompoundStmt(getSourceSpan(ctx), scope::StorageKind::Global);
        auto module = new ast::Module(moduleName_, compoundStmt);
        for (LwnnParser::StmtContext *stmt: statements) {
            StmtListener listener;
            stmt->enterRule(&listener);
            if(!listener.hasResult()) return;
            module->body()->addStmt(listener.surrenderResult());
        }
        setResult(module);
    }
};

/*******************************************/
class LwnnErrorListener : public BaseErrorListener {
    error::ErrorStream &errorStream_;
    std::string inputName_;
public:
    LwnnErrorListener(error::ErrorStream &errorStream_)
        : errorStream_(errorStream_) {}

    virtual void syntaxError(Recognizer *, Token *offendingSymbol, size_t line,
                             size_t charPositionInLine, const std::string &msg, std::exception_ptr) {

        //If offendingSymbol is null, error was from lexer and we don't even have an error token yet.
        if(offendingSymbol == nullptr) {
            //We can create a new source span--this one will only have a length of 1, though...
            source::SourceSpan span {
                inputName_,
                source::SourceLocation { line, charPositionInLine },
                source::SourceLocation { line, charPositionInLine + 1 }
            };
            errorStream_.error(error::ErrorKind::Syntax, span, msg);
        } else {
            //However if we do have the offendingSymbol we should use that because the span generated
            //from it should have the span's full length.
            errorStream_.error(error::ErrorKind::Syntax, getSourceSpan(offendingSymbol), msg);
        }
    };

    virtual void reportAmbiguity(Parser *, const dfa::DFA &, size_t , size_t , bool ,
                                 const antlrcpp::BitSet &, atn::ATNConfigSet *) override {
        // May want to grab code from here to determine how to report this intelligently
        // https://github.com/antlr/antlr4/blob/master/runtime/Cpp/runtime/src/DiagnosticErrorListener.cpp
//                source::SourceSpan span {
//                    inputName_,
//                    source::SourceLocation { 1, 1},
//                    source::SourceLocation { 1, 1 }
//                };
//                errorStream_.warning(span, "Grammar is ambiguous.  I think this is an bug in Lwnn.g4.");
    }

    virtual void reportAttemptingFullContext(Parser *, const dfa::DFA &, size_t , size_t ,
                                             const antlrcpp::BitSet &, atn::ATNConfigSet *) override  {
    }

    virtual void reportContextSensitivity(Parser *, const dfa::DFA &, size_t , size_t ,
                                          size_t , atn::ATNConfigSet *) override  {
    }
};

Module *parseModule(const std::string &lineOfCode, const std::string &inputName) {
    error::ErrorStream errorStream{std::cerr};
    LwnnErrorListener errorListener{errorStream};

    ANTLRInputStream inputStream{lineOfCode};
    LwnnLexer lexer{&inputStream};
    inputStream.name = inputName;

    lexer.removeErrorListeners();
    lexer.addErrorListener(&errorListener);

    CommonTokenStream tokens{&lexer};
    tokens.fill();

    if(errorStream.errorCount() > 0) return nullptr;

    LwnnParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&errorListener);

    ModuleListener listener{inputName};

    auto *moduleCtx = parser.module();

    moduleCtx->enterRule(&listener);
    if(errorStream.errorCount() > 0) return nullptr;
    if(!listener.hasResult()) {
        std::cerr << "Nothing was parsed!\n";
        return nullptr;
    }

    return listener.surrenderResult();
}

}}
