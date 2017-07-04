#include <iostream>
#include <memory>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "_Wattributes"
#include <antlr4-runtime.h>
#pragma GCC diagnostic pop

#include "generated/HelloLexer.h"
#include "generated/HelloParser.h"
//#include "generated/HelloBaseVisitor.h"
//#include "generated/HelloVisitor.h"
#include "generated/HelloBaseListener.h"

using namespace LwnnParser;
using namespace antlr4;

// Simplest grammar I could find:  https://gist.github.com/mattmcd/5425206
// Stuff I should read: http://www.soft-gems.net/index.php/tools/49-the-antlr4-c-target-is-here

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

int main() {
	std::cout << "Hello, World!" << std::endl;

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

	return 0;
}
