#include <antlr4-runtime.h>

#include "generated/HelloLexer.h"
#include "generated/HelloParser.h"
//#include "generated/HelloBaseVisitor.h"
//#include "generated/HelloVisitor.h"
#include "generated/HelloBaseListener.h"

using namespace LwnnParser;
using namespace antlr4;

class HelloWalker : public HelloBaseListener
{
public:
    void enterR(HelloParser::RContext* ctx)
    {
        std::cout << "Entering R : " << ctx->ID()->getText() << std::endl;
    }

    void exitR(HelloParser::RContext *ctx)
    {
        std::cout << "Exiting R : " << ctx->ID()->getText() << std::endl;
    }
};

void do_antlr4_demo()
{
    ANTLRInputStream inputStream("hello world");
    HelloLexer lexer(&inputStream);
    CommonTokenStream tokens(&lexer);

    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }

    HelloParser parser(&tokens);

    HelloParser::RContext *tree = parser.r();
    tree::ParseTreeWalker walker;

    std::cout << "Walking the tree\n";
    auto helloWalker = std::make_unique<HelloWalker>();
    walker.walk(helloWalker.get(), tree);
    std::cout << "Done walking the tree\n";

    std::cout << "Here's a string tree:\n";
    std::cout << tree->toStringTree(&parser) << std::endl << std::endl;

    std::cout << "End of ANTLR4 demo\n\n";
    std::cout.flush();
}
