
#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "../front/parser/AnodeLexer.h"
#include <limits>

#include "common/containers.h"

#include <sstream>

using namespace anode;
using namespace anode::front::parser;


gc_vector<Token*> extractAllTokens(const std::string &fromStr) {
    std::stringstream inputStream{fromStr};
    SourceReader reader{"integration_test", inputStream};
    error::ErrorStream errorStream{std::cerr};
    AnodeLexer lexer{reader, errorStream};

    gc_vector<Token*> tokens;
    Token *t;
    do {
        t = lexer.nextToken();
        tokens.push_back(t);
    } while(t->kind() != TokenKind::END_OF_INPUT);

    return tokens;
}

TEST_CASE("simple tokens") {
    auto tokens = extractAllTokens("; ! + - * / = == != > < >= <= ++ -- . : ( ) { } true false while if func cast class assert new");
    int i = 0;
    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_STATEMENT);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_NOT);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_ADD);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_SUB);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_MUL);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_DIV);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_ASSIGN);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_EQ);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_NEQ);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_GT);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_LT);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_GTE);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_LTE);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_INC);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_DEC);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_DOT);
    REQUIRE(tokens[i++]->kind() == TokenKind::OP_DEF);
    REQUIRE(tokens[i++]->kind() == TokenKind::OPEN_PAREN);
    REQUIRE(tokens[i++]->kind() == TokenKind::CLOSE_PAREN);
    REQUIRE(tokens[i++]->kind() == TokenKind::OPEN_CURLY);
    REQUIRE(tokens[i++]->kind() == TokenKind::CLOSE_CURLY);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_TRUE);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_FALSE);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_WHILE);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_IF);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_FUNC);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_CAST);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_CLASS);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_ASSERT);
    REQUIRE(tokens[i++]->kind() == TokenKind::KW_NEW);
    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);

    REQUIRE(i == tokens.size());
}

TEST_CASE("identifiers") {
    auto tokens = extractAllTokens("a abcdef zxyw_lmnop a123 _abc abc_ ");
    int i = 0;
    Token *token = tokens[i++];
    REQUIRE(token->text() == "a");
    REQUIRE(token->kind() == TokenKind::ID);

    token = tokens[i++];
    REQUIRE(token->text() == "abcdef");
    REQUIRE(token->kind() == TokenKind::ID);

    token = tokens[i++];
    REQUIRE(token->text() == "zxyw_lmnop");
    REQUIRE(token->kind() == TokenKind::ID);

    token = tokens[i++];
    REQUIRE(token->text() == "a123");
    REQUIRE(token->kind() == TokenKind::ID);

    token = tokens[i++];
    REQUIRE(token->text() == "_abc");
    REQUIRE(token->kind() == TokenKind::ID);

    token = tokens[i++];
    REQUIRE(token->text() == "abc_");
    REQUIRE(token->kind() == TokenKind::ID);


    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);
    REQUIRE(i == tokens.size());
}


TEST_CASE("literal integers") {
    std::string helper;
    auto tokens = extractAllTokens("0 -1 1 1024 -1024 2147483647 -2147483647");

    int i = 0;
    Token *token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 0);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == -1);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 1);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 1024);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == -1024);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 2147483647);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == -2147483647);

    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);
    REQUIRE(i == tokens.size());
}


TEST_CASE("literal float") {
    std::string helper;
    auto tokens = extractAllTokens("0.0 -1.0 1.0 1024.0 -1024.0");

    int i = 0;
    Token *token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_FLOAT);
    REQUIRE(token->floatValue() == 0.0);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_FLOAT);
    REQUIRE(token->floatValue() == -1.0);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_FLOAT);
    REQUIRE(token->floatValue() == 1.0);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_FLOAT);
    REQUIRE(token->floatValue() == 1024.0);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_FLOAT);
    REQUIRE(token->floatValue() == -1024.0);

    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);
    REQUIRE(i == tokens.size());
}

TEST_CASE("single line comment") {
    auto tokens = extractAllTokens("1\n # 10 single 20 line 30 comment \n2");

    int i = 0;
    Token *token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 1);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 2);

    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);
    REQUIRE(i == tokens.size());

}

TEST_CASE("multiple line comment") {
    auto tokens = extractAllTokens("1\n (# 10 multiple line comment \n(# nested comment #) #) (#(#(#(# \n (#10 \n #)#)\t#)\n#)#)\n2");

    int i = 0;
    Token *token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 1);

    token = tokens[i++];
    REQUIRE(token->kind() == TokenKind::LIT_INT);
    REQUIRE(token->intValue() == 2);

    REQUIRE(tokens[i++]->kind() == TokenKind::END_OF_INPUT);
    REQUIRE(i == tokens.size());

}