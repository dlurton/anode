
#include "Lexer.h"

namespace lwnn { namespace front { namespace parser {

std::unordered_map<char_t, TokenKind> SingleCharacterTokens =
{
    { ';', TokenKind::END_OF_STATEMENT },
    { '!', TokenKind::OP_NOT },
    { '+', TokenKind::OP_ADD },
    { '-', TokenKind::OP_SUB },
    { '*', TokenKind::OP_MUL },
    { '/', TokenKind::OP_DIV },
    { '=', TokenKind::OP_ASSIGN },
    { '>', TokenKind::OP_GT },
    { '>', TokenKind::OP_DIV },
    { '<', TokenKind::OP_LT },
    { '.', TokenKind::OP_DOT },
    { ':', TokenKind::OP_DEF },
    { '(', TokenKind::OPEN_PAREN },
    { ')', TokenKind::CLOSE_PAREN },
    { '{', TokenKind::OPEN_CURLY },
    { '}', TokenKind::CLOSE_CURLY },
    { ',', TokenKind::COMMA },
};

//These will be evaluated in the order the are specified.
std::vector<std::pair<string_t, TokenKind>> MultiCharacterTokens =
{
    { "++", TokenKind::OP_INC },
    { "--", TokenKind::OP_DEC },
    { "==", TokenKind::OP_EQ },
    { "!=", TokenKind::OP_NEQ },
    { "&&", TokenKind::OP_LAND },
    { "||", TokenKind::OP_LOR },
    { ">=", TokenKind::OP_GTE },
    { "<=", TokenKind::OP_LTE },
    { "(?", TokenKind::OP_COND },
    { "true", TokenKind::KW_TRUE },
    { "false", TokenKind::KW_FALSE},
    { "while", TokenKind::KW_WHILE },
    { "if", TokenKind::KW_IF },
    { "else", TokenKind::KW_ELSE },
    { "func", TokenKind::KW_FUNC },
    { "cast", TokenKind::KW_CAST },
    { "class", TokenKind::KW_CLASS},
    { "assert", TokenKind::KW_ASSERT},
};

Token *Lexer::extractLiteralNumber() {
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

Token *Lexer::extractIdentifier() {
    std::string id;
    id += reader_.next();
    char_t c = reader_.peek();
    while((isLetter(c) || isDigit(c) || c == '_') && !reader_.eof()) {
        id += reader_.next();
        c = reader_.peek();
    }
    return newToken(TokenKind::ID, id);
}

Token *Lexer::extractToken() {
    discardWhite();
    markTokenStart();

    if(reader_.eof()) {
        return newToken(TokenKind::END_OF_INPUT, "<EOF>");
    }

    //Search for a match among the multi-character tokens
    //(because some of the multi-character tokens will start with the same character as a single-character token.)
    for(std::pair<string_t, TokenKind> multiCharToken : MultiCharacterTokens) {
        if(reader_.match(multiCharToken.first)) {
            return newToken(multiCharToken.second, multiCharToken.first);
        }
    }

    char_t c = reader_.peek();

    if(c == '-' && isDigit(reader_.peek(1))) {
        return extractLiteralNumber();
    }

    auto found = SingleCharacterTokens.find(c);
    if(found != SingleCharacterTokens.end()) {
        return newToken(found->second, reader_.next());
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

bool Lexer::discardMultilineComment() {
    if(!reader_.match("(#")) return false;

    int nestDepth = 1;
    while(nestDepth > 0) {
        if(reader_.eof()) {
            errorUnexpectedEofInMultilineComment();
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

void Lexer::discardWhite() {
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

bool Lexer::discardSingleLineComment() {
    if(!reader_.match("#")) return false;

    while(reader_.peek() != '\n' && !reader_.eof()) {
        reader_.next();
    }

    reader_.next(); //Discard '\n'

    return true;
}
}}}