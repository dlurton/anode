#include "catch.hpp"
#include "../front/LwnnParser2.h"
#include "front/visualize.h"

#include <sstream>

using namespace lwnn;
using namespace lwnn::ast;
using namespace lwnn::front::parser;

ast::ExprStmt *parseExprStmt(const std::string &source) {
    std::cerr << "Parsing: " << source << "\n";

    std::stringstream inputStream{source};
    SourceReader reader{"integration_test", inputStream};
    error::ErrorStream errorStream{std::cerr};
    Lexer lexer{reader, errorStream};

    LwnnParser parser{lexer, errorStream};

    ExprStmt *expr = parser.parseExprStmt();

    if(errorStream.errorCount() > 0) {
        FAIL("Parse error encountered.");
    }

    lwnn::visualize::prettyPrint(expr);

    return expr;
}

ast::Module *parseModule(const std::string &source) {
    std::cerr << "Parsing: " << source << "\n";

    std::stringstream inputStream{source};
    SourceReader reader{"integration_test", inputStream};
    error::ErrorStream errorStream{std::cerr};
    Lexer lexer{reader, errorStream};

    LwnnParser parser{lexer, errorStream};

    Module *expr = parser.parseModule();

    if(errorStream.errorCount() > 0) {
        FAIL("Parse error encountered.");
    }

    lwnn::visualize::prettyPrint(expr);

    return expr;
}

TEST_CASE("parse simple expressions") {
    auto expr = parseExprStmt("1;");
    auto literalInt32 = upcast<LiteralInt32Expr>(expr);
    REQUIRE(literalInt32->value() == 1);

    expr = parseExprStmt("1.0;");
    auto literalFloat = upcast<LiteralFloatExpr>(expr);
    REQUIRE(literalFloat->value() == 1.0);

    expr = parseExprStmt("a;");
    auto variableRefExpr = upcast<VariableRefExpr>(expr);
    REQUIRE(variableRefExpr->name() == "a");
}

TEST_CASE("parse unary expressions") {
    auto expr = parseExprStmt("!1;");
    auto unaryExpr = ast::upcast<UnaryExpr>(expr);
    REQUIRE(unaryExpr->operation() == UnaryOperationKind::Not);
    auto literalInt32 = upcast<LiteralInt32Expr>(unaryExpr->valueExpr());
    REQUIRE(literalInt32->value() == 1);
}

TEST_CASE("parse binary expressions") {
    ast::ExprStmt* expr = parseExprStmt("{ 1 + 2; }");
    auto binaryExpr = ast::upcast<BinaryExpr>(expr);
    REQUIRE(binaryExpr->operation() == BinaryOperationKind::Add);

    auto literalInt32 = upcast<LiteralInt32Expr>(binaryExpr->lValue());
    REQUIRE(literalInt32->value() == 1);

    literalInt32 = upcast<LiteralInt32Expr>(binaryExpr->rValue());
    REQUIRE(literalInt32->value() == 2);
}

TEST_CASE("parse some stuff") {
//    parseExprStmt("cast<int>(someValue);");
//
//    parseExprStmt("{ 1; 2; 3; }");
//    try  {
//        parseExprStmt("1 * 2 + 3;");
//    }
//    catch(std::runtime_error e) {
//        std::cerr << e.what();
//    }
//
//    parseExprStmt("1 * (2 + 3);");
//    parseExprStmt("1.0 * (2.0 + 3.0);");
//    parseExprStmt("someVariable = 1;");
//    parseExprStmt("someVariable:int = 1;");
//    parseExprStmt("{ true; false; }");
//    parseExprStmt("(? 1, (? 2, 3, 4), 5);");

//    parseExprStmt("if(true) 1; else 2;");
//    parseExprStmt("if(true) { 1; } else 2;");
//    parseExprStmt("if(true) 1; else { 2; }");
//    parseExprStmt("if(true) { 1; } else { 2; }");
//    parseExprStmt("if(true) { 1; } else if(false) { 2; } else { 3; }");
//    parseExprStmt("if(true) if(false) { 2; } else { 3; }");

//    parseExprStmt("while(true) a;");
//    parseExprStmt("while(true) while(false) { a; }");
//    parseExprStmt("someFunc();");
//    parseExprStmt("someFunc(a);");
//    parseExprStmt("someFunc(a, b);");
//    parseExprStmt("someFunc(a, b(c));");

//    parseExprStmt("func foo:int() 1;");
//    parseExprStmt("func foo:int() { 1; }");
//    parseExprStmt("func foo:int(someParam1:int) { 1; }");
//    parseExprStmt("func foo:int(someParam1:int, someParam2:int) { 1; }");

     parseExprStmt("class foo bar:int;");
     parseExprStmt("class foo { bar:int; baT:float; }");
     parseExprStmt(
         R"(
    class foo {
        bar:int; baT:float;
        func foo:int(bar:float) 1;
        func someFunc:int() {
            class bloo {
                amatuer:hour;
            }
        }
     }
    )");
}

TEST_CASE("parse module") {
    parseModule("1; 2; 3; class foo { bar:int; }");
}

TEST_CASE("parse assert") {
    FAIL("do me!");
}