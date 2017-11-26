
#include <front/parse.h>
#include "AnodeLexer.h"

namespace anode { namespace front { namespace parser {

/** Static token is the name I have chosen to indicate tokens that do not ever change. i.e. operators
 * and keywords, etc.  Literal values, identifiers, etc would be dynamic. */
std::vector<std::vector<std::pair<string_t, TokenKind>>> StaticTokenLookup;

void registerStaticToken(string_t text, TokenKind tokenKind) {
    StaticTokenLookup.resize(MAX_CHAR, std::vector<std::pair<string_t, TokenKind>>());
    char_t firstChar = text[0];
    StaticTokenLookup[firstChar].emplace_back(text, tokenKind);
}

void InitStaticTokenLookup() {
    if(StaticTokenLookup.size() > 0) { 
        return;
    }

    //For tokens that start with the same character(s), the longer one must be registered first!
    registerStaticToken("++", TokenKind::OP_INC);
    registerStaticToken("--", TokenKind::OP_DEC);
    registerStaticToken("==", TokenKind::OP_EQ);
    registerStaticToken("!=", TokenKind::OP_NEQ);
    registerStaticToken("&&", TokenKind::OP_LAND);
    registerStaticToken("||", TokenKind::OP_LOR);
    registerStaticToken(">=", TokenKind::OP_GTE);
    registerStaticToken("<=", TokenKind::OP_LTE);
    registerStaticToken("(?", TokenKind::OP_COND);
    registerStaticToken("true", TokenKind::KW_TRUE);
    registerStaticToken("false", TokenKind::KW_FALSE);
    registerStaticToken("while", TokenKind::KW_WHILE);
    registerStaticToken("if", TokenKind::KW_IF);
    registerStaticToken("else", TokenKind::KW_ELSE);
    registerStaticToken("func", TokenKind::KW_FUNC);
    registerStaticToken("cast", TokenKind::KW_CAST);
    registerStaticToken("new", TokenKind::KW_NEW);
    registerStaticToken("class", TokenKind::KW_CLASS);
    registerStaticToken("assert", TokenKind::KW_ASSERT);
    registerStaticToken("alias", TokenKind::KW_ALIAS);
    registerStaticToken("expand", TokenKind::KW_EXPAND);
    registerStaticToken("template", TokenKind::KW_TEMPLATE);
    registerStaticToken(";", TokenKind::END_OF_STATEMENT);
    registerStaticToken("!", TokenKind::OP_NOT);
    registerStaticToken("+", TokenKind::OP_ADD);
    registerStaticToken("-", TokenKind::OP_SUB);
    registerStaticToken("*", TokenKind::OP_MUL);
    registerStaticToken("/", TokenKind::OP_DIV);
    registerStaticToken("=", TokenKind::OP_ASSIGN);
    registerStaticToken(">", TokenKind::OP_GT);
    registerStaticToken(">", TokenKind::OP_DIV);
    registerStaticToken("<", TokenKind::OP_LT);
    registerStaticToken(".", TokenKind::OP_DOT);
    registerStaticToken(":", TokenKind::OP_DEF);
    registerStaticToken("(", TokenKind::OPEN_PAREN);
    registerStaticToken(")", TokenKind::CLOSE_PAREN);
    registerStaticToken("{", TokenKind::OPEN_CURLY);
    registerStaticToken("}", TokenKind::CLOSE_CURLY);
    registerStaticToken(",", TokenKind::COMMA);
}

Token &AnodeLexer::extractLiteralNumber() {
    string_t number;
    number += reader_.next();
    int c = reader_.peek();
    while ((isDigit(c) || c == '.') && !reader_.eof()) {
        number += reader_.next();
        c = reader_.peek();
    }

    if (number.find('.') == string_t::npos) {
        //It's an integer
        if (!isValidInt(number)) {
            errorStream_.error(
                error::ErrorKind::InvalidLiteralInt32,
                getSourceSpanForCurrentToken(),
                string::format("Invalid literal int '%s'", number.c_str()));

            return newToken(TokenKind::UNEXPECTED, to_string(c));
        }
        return newToken(TokenKind::LIT_INT, number);
    } else {
        //it's a float
        if (!isValidFloat(number)) {
            errorStream_.error(
                error::ErrorKind::InvalidLiteralFloat,
                getSourceSpanForCurrentToken(),
                string::format("Invalid literal float '%s'", number.c_str()));

            return newToken(TokenKind::UNEXPECTED, to_string(c));
        }
        return newToken(TokenKind::LIT_FLOAT, number);
    }
}

Token &AnodeLexer::extractIdentifier() {
    std::string id;
    id += reader_.next();
    char_t c = reader_.peek();
    while((isLetter(c) || isDigit(c) || c == '_') && !reader_.eof()) {
        id += reader_.next();
        c = reader_.peek();
    }
    return newToken(TokenKind::ID, id);
}

Token &AnodeLexer::extractToken() {
    discardWhite();
    markTokenStart();

    if(reader_.eof()) {
        return newToken(TokenKind::END_OF_INPUT, "<EOF>");
    }

    char_t c = reader_.peek();
    if(c == '-' && isDigit(reader_.peek(1))) {
        return extractLiteralNumber();
    }
    const auto &candidates = StaticTokenLookup[c];
    for(const std::pair<string_t, TokenKind> &multiCharToken : candidates) {
        if(reader_.match(multiCharToken.first)) {
            return newToken(multiCharToken.second, multiCharToken.first);
        }
    }

    //Extract an identifier
    if(isLetter(c) || c == '_') {
        return extractIdentifier();
    }

    //Extract a number
    if(isDigit(c)) {
        return extractLiteralNumber();
    }

    errorStream_.error(
        error::ErrorKind::UnexpectedCharacter,
        getSourceSpanForCurrentToken(),
        string::format(
            "Invalid character '%s' (0x%X)", to_string(c).c_str(), (unsigned int)c));

    reader_.next();

    return newUnexpectedToken(c);
}

bool AnodeLexer::discardMultilineComment() {

    SourceLocation startLocation = reader_.getCurrentSourceLocation();
    if(!reader_.match("(#")) return false;

    int nestDepth = 1;
    while(nestDepth > 0) {
        if(reader_.eof()) {
            errorStream_.error(
                error::ErrorKind::UnexpectedEofInMultilineComment,
                SourceSpan(reader_.inputName(), startLocation, reader_.getCurrentSourceLocation()),
                "Unexpected end-of-input within multi-line comment");
            throw ParseAbortedException();
        }

        if(reader_.match("(#")) {
            ++nestDepth;
            continue;
        }

        if(reader_.match("#)")) {
            --nestDepth;
            continue;
        }
        reader_.next();
    }
    return true;
}

void AnodeLexer::discardWhite() {
    while(true) {
        if(discardSingleLineComment()) {
            continue;
        }

        if(discardMultilineComment()) {
            continue;
        }

        if(discardWhitespaceCharacters()) {
            continue;
        }

        break;
    }
}

bool AnodeLexer::discardSingleLineComment() {
    if(!reader_.match("#")) return false;

    while(reader_.peek() != '\n' && !reader_.eof()) {
        reader_.next();
    }

    reader_.next(); //Discard '\n'

    return true;
}
}}}