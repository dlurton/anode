#include <antlr4-runtime.h>

#include "generated/LwnnLexer.h"
#include "generated/LwnnParser.h"
//#include "generated/HelloBaseVisitor.h"
//#include "generated/HelloVisitor.h"
#include "generated/LwnnBaseListener.h"

using namespace LwnnGeneratedParser;
using namespace antlr4;

class LwnnToAstListener : public LwnnBaseListener
{
public:
    virtual void enterProgram(LwnnParser::ProgramContext * ctx) override {
        std::cout << "enterProgram:" << ctx->getText() << "\n";
    }
    virtual void exitProgram(LwnnParser::ProgramContext *ctx) override {
        std::cout << "exitProgram:" << ctx->getText() << "\n";
    }

    virtual void enterStatement(LwnnParser::StatementContext * ctx) override {
        std::cout << "enterStatement:" << ctx->getText() << "\n";
    }
    virtual void exitStatement(LwnnParser::StatementContext *ctx) override {
        std::cout << "exitStatement:" << ctx->getText() << "\n";
    }

    virtual void enterParen_expr(LwnnParser::Paren_exprContext *ctx) override {
        std::cout << "enterParen_expr:" << ctx->getText() << "\n";
    }
    virtual void exitParen_expr(LwnnParser::Paren_exprContext *ctx) override {
        std::cout << "exitParen_expr:" << ctx->getText() << "\n";
    }

    virtual void enterExpr(LwnnParser::ExprContext *ctx) override {
        std::cout << "enterExpr:" << ctx->getText() << "\n";
    }
    virtual void exitExpr(LwnnParser::ExprContext *ctx) override {
        std::cout << "exitExpr:" << ctx->getText() << "\n";
    }

    virtual void enterTest(LwnnParser::TestContext *ctx) override {
        std::cout << "exitExpr:" << ctx->getText() << "\n";
    }
    virtual void exitTest(LwnnParser::TestContext *ctx) override {
        std::cout << "exitTest:" << ctx->getText() << "\n";
    }

    virtual void enterSum(LwnnParser::SumContext *ctx) override {
        std::cout << "enterSum:" << ctx->getText() << "\n";
    }
    virtual void exitSum(LwnnParser::SumContext *ctx) override {
        std::cout << "exitSum:" << ctx->getText() << "\n";
    }

    virtual void enterTerm(LwnnParser::TermContext *ctx) override {
        std::cout << "enterTerm:" << ctx->getText() << "\n";
    }
    virtual void exitTerm(LwnnParser::TermContext *ctx) override {
        std::cout << "exitTerm:" << ctx->getText() << "\n";
    }

    virtual void enterId(LwnnParser::IdContext *ctx) override {
        std::cout << "enterId:" << ctx->getText() << "\n";
    }
    virtual void exitId(LwnnParser::IdContext *ctx) override {
        std::cout << "exitId:" << ctx->getText() << "\n";
    }

    virtual void enterInteger(LwnnParser::IntegerContext * ctx) override {
        std::cout << "enterInteger:" << ctx->getText() << "\n";
    }

    virtual void exitInteger(LwnnParser::IntegerContext *ctx) override {
        std::cout << "exitInteger:" << ctx->getText() << "\n";
    }

//    virtual void enterEveryRule(antlr4::ParserRuleContext *ctx) override {
//        std::cout << "enterEveryRule:" << ctx->getText() << "\n";
//    }
//    virtual void exitEveryRule(antlr4::ParserRuleContext *ctx) override {
//        std::cout << "exitEveryRule:" << ctx->getText() << "\n";
//    }
//    virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override {
//        std::cout << "visitTerminal:" << ctx->getText() << "\n";
//    }
//    virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override {
//        std::cout << "visitErrorNode:" << ctx->getText() << "\n";
//    }
};

void do_antlr4_demo(const char *lineOfCode)
{
    ANTLRInputStream inputStream(lineOfCode);
    LwnnLexer lexer(&inputStream);
    CommonTokenStream tokens(&lexer);

    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }

    LwnnParser parser(&tokens);

    LwnnParser::ProgramContext *tree = parser.program();
    tree::ParseTreeWalker walker;

    std::cout << "Walking the tree\n";
    auto lwnnWalker = std::make_unique<LwnnToAstListener>();
    walker.walk(lwnnWalker.get(), tree);
    std::cout << "Done walking the tree\n";

    std::cout << "Here's a string tree:\n";
    std::cout << tree->toStringTree(&parser) << std::endl << std::endl;

    std::cout << "End of ANTLR4 demo\n\n";
    std::cout.flush();
}
