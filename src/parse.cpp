#include "AST.hpp"
#include "PrettyPrinter.hpp"
#include "ExpressionTreeWalker.hpp"

#include <antlr4-runtime.h>
#include "generated/LwnnLexer.h"
#include "generated/LwnnParser.h"
#include "generated/LwnnBaseListener.h"

using namespace LwnnGeneratedParser;
using namespace antlr4;

namespace lwnn {

    /** Extracts a SourceRange from values specified in ctx. */
    static SourceSpan getSourceRange(antlr4::ParserRuleContext *ctx) {
        auto startSource = ctx->getStart()->getTokenSource();
        auto endSource = ctx->getStop()->getTokenSource();

        return SourceSpan(startSource->getSourceName(),
                           SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
                           SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
    }

    /** Extracts a SourceRange from values specified in ctx. */
    static SourceSpan getSourceRange(antlr4::ParserRuleContext *startContext, antlr4::ParserRuleContext *endContext) {
        auto startSource = startContext->getStart()->getTokenSource();
        auto endSource = endContext->getStop()->getTokenSource();

        return SourceSpan(startSource->getSourceName(),
                           SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
                           SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
    }

    class ExpressionListener : public LwnnBaseListener {
    private:
        std::unique_ptr<const Expr> expr_;
    public:
        virtual void enterParensExpr(LwnnParser::ParensExprContext *ctx) override {
            ExpressionListener listener;
            ctx->expr()->enterRule(&listener);
            expr_ = listener.surrenderExpr();
        }

        virtual void enterNumberExpr(LwnnParser::NumberExprContext *ctx) override {
            int value = std::stoi(ctx->getText());
            expr_ = std::make_unique<LiteralInt32>(getSourceRange(ctx), value);
        }

        virtual void enterInfixExpr(LwnnParser::InfixExprContext *ctx) override {
            ExpressionListener leftListener;
            ctx->left->enterRule(&leftListener);

            ExpressionListener rightListener;
            ctx->right->enterRule(&rightListener);

            OperationKind opKind;

            switch(ctx->op->getType()) {
                case LwnnParser::OP_ADD: opKind = OperationKind::Add; break;
                case LwnnParser::OP_SUB: opKind = OperationKind::Sub; break;
                case LwnnParser::OP_MUL: opKind = OperationKind::Mul; break;
                case LwnnParser::OP_DIV: opKind = OperationKind::Div; break;
                default:
                    throw new UnhandledSwitchCase();
            }

            expr_ = std::make_unique<const Binary>(getSourceRange(ctx->left, ctx->right),
                leftListener.surrenderExpr(), opKind, rightListener.surrenderExpr());
        }

        virtual void enterVarRefExpr(LwnnParser::VarRefExprContext * ctx) override {
            expr_ = std::make_unique<VariableRef>(getSourceRange(ctx), ctx->getText());
        }

        std::unique_ptr<const Expr> surrenderExpr() {
            return std::move(expr_);
        }
    };

    class CompiledUnitListener : public LwnnBaseListener {
    private:
        std::unique_ptr<const Expr> expr_;
    public:
        virtual void enterCompileUnit(LwnnParser::CompileUnitContext *ctx) override {
            ExpressionListener listener;
            ctx->expr()->enterRule(&listener);
            expr_ = listener.surrenderExpr();
        }

        std::unique_ptr<const Expr> surrenderExpr() {
            return std::move(expr_);
        }
    };


    void parse(const char *lineOfCode) {
        ANTLRInputStream inputStream(lineOfCode);
        LwnnLexer lexer(&inputStream);
        CommonTokenStream tokens(&lexer);
        tokens.fill();

        LwnnParser parser(&tokens);
        auto *compileUnitCtx = parser.compileUnit();

        CompiledUnitListener listener;
        listener.enterCompileUnit(compileUnitCtx);

        std::unique_ptr<const Expr> expr = listener.surrenderExpr();

        //Gotta make a fake module & function for now...
        PrettyPrinterVisitor visitor(std::cout);
        ExpressionTreeWalker walker(&visitor);

        walker.walk(expr.get());

        std::cout << "\n\nEnd of ANTLR4 demo\n\n";
        std::cout.flush();
    }
}