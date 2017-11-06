
#include "AnodeLexer.h"
#include "front/ast.h"
#include "front/parse.h"

#pragma once

namespace anode { namespace front { namespace parser {

/** Defines a function signature which is invoked by PrattParser when it encounters a specific symbol.
 * Such as a prefix operator (i.e. ++i), or a keyword (i.e. 'if' or 'while') to parse the appropriate language construct.
 */
typedef std::function<ast::ExprStmt*(Token*)> generic_parselet_t;

/** Like generic_parselet_t, but for parsing infix expressions (a.k.a binary expressions, i.e. 1 + 1). */
typedef std::function<ast::ExprStmt*(ast::ExprStmt*, Token*)> infix_parselet_t;

/** Operator associativity. */
enum class Associativity {
    Left,
    Right
};


/** A basic Pratt Parser.
 * http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
 *
 * @tparam TExpression
 */
template<typename TExpression>
class PrattParser {
    gc_vector<generic_parselet_t> genericParsers_;
    gc_vector<infix_parselet_t> infixParsers_;

protected:
    error::ErrorStream &errorStream_;
    AnodeLexer &lexer_;

    PrattParser (AnodeLexer &lexer, error::ErrorStream &errorStream) : errorStream_{errorStream}, lexer_{lexer} {
        genericParsers_.resize((int) TokenKind::MAX_TOKEN_TYPES, generic_parselet_t());
        infixParsers_.resize((int)TokenKind::MAX_TOKEN_TYPES, infix_parselet_t());
    }

    void registerGenericParselet(TokenKind tokenKind, generic_parselet_t parselet) {
        if(genericParsers_[(int)tokenKind] != nullptr) {
            ASSERT_FAIL("Specified tokenKind already exists in genericParsers_.")
        }
        genericParsers_[(int)tokenKind] = parselet;
    }

    void registerInfixParselet(TokenKind tokenKind, infix_parselet_t parselet) {
        if(infixParsers_[(int)tokenKind] != nullptr) {
            ASSERT_FAIL("Specified tokenKind already exists in infixParsers_.")
        }
        infixParsers_[(int)tokenKind] = parselet;
    }

    Token *consume(TokenKind tokenKind, char_t expectedCharacter) {
        string_t msg;
        msg += '\'';
        msg += expectedCharacter;
        msg += '\'';

        return consume(tokenKind, msg);
    }

    Token *consume(TokenKind tokenKind1, TokenKind tokenKind2, char_t expectedCharacter1, char_t expectedCharacter2) {
        string_t msg;
        msg += '\'';
        msg += expectedCharacter1;
        msg += "' or '";
        msg += expectedCharacter2;
        msg += '\'';

        return consume(tokenKind1, tokenKind2, msg);
    }

    Token *consume(TokenKind tokenKind1, TokenKind tokenKind2, const std::string &humanReadableExpectedText) {
        Token *token = lexer_.nextToken();
        if(token->kind() != tokenKind1 && token->kind() != tokenKind2) {
            errorStream_.error(
                error::ErrorKind::UnexpectedToken,
                token->span(),
                "Expected " + humanReadableExpectedText);

            throw ParseAbortedException();
        }
        return token;
    }

    Token *consume(TokenKind tokenKind, const std::string &humanReadableExpectedText) {
        Token *token = lexer_.nextToken();
        if(token->kind()!= tokenKind) {
            errorStream_.error(
                error::ErrorKind::UnexpectedToken,
                token->span(),
                "Expected " + humanReadableExpectedText);

            throw ParseAbortedException();
        }
        return token;
    }

    Token *consumeOptional(TokenKind tokenKind) {
        if(lexer_.peekToken()->kind() == tokenKind) {
            return lexer_.nextToken();
        }
        return nullptr;
    }


    virtual int getOperatorPrecedence(TokenKind kind) = 0;

    TExpression *parseExpr() {
        return parseExpr(0);
    }

    TExpression *parseExpr(int precedence) {

        Token *t = lexer_.nextToken();
        if(t->kind() == TokenKind::END_OF_INPUT) {
            throw ParseAbortedException("Unexpected end of input!");
        }

        const char *message = "The token '%s' came as a complete surprise to me.";

        auto foundPrefixParselet = genericParsers_[(int)t->kind()];
        if(foundPrefixParselet == nullptr) {
            errorStream_.error(
                error::ErrorKind::SurpriseToken,
                t->span(),
                message,
                t->text().c_str());
            throw ParseAbortedException();
        }

        ast::ExprStmt *left = foundPrefixParselet(t);

        t = lexer_.peekToken();
        if(t->kind() == TokenKind::END_OF_INPUT) {
            return left;
        }

        while(precedence < getOperatorPrecedence(t->kind())) {
            lexer_.nextToken();
            auto foundInfixParselet = infixParsers_[(int)t->kind()];
            if(foundInfixParselet == nullptr) {
                errorStream_.error(
                    error::ErrorKind::SurpriseToken,
                    t->span(),
                    message,
                    t->text().c_str());
            }

            left = foundInfixParselet(left, t);

            t = lexer_.peekToken();
            if(t->kind() == TokenKind::END_OF_INPUT) {
                return left;
            }
        }

        return left;
    }
};

}}}