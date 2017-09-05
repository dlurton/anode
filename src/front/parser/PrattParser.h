
#include "Lexer.h"
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
    gc_unordered_map<TokenKind, generic_parselet_t> prefixParseletMap_;
    gc_unordered_map<TokenKind, infix_parselet_t> infixParseletMap_;
protected:
    error::ErrorStream &errorStream_;
    Lexer &lexer_;

    void registerGenericParselet(TokenKind tokenKind, generic_parselet_t parselet) {
        auto found = prefixParseletMap_.find(tokenKind);
        if(found != prefixParseletMap_.end()) {
            ASSERT_FAIL("Specified tokenKind already exists in prefixParseletMap_.")
        }
        prefixParseletMap_[tokenKind] = parselet;
    }

    void registerInfixParselet(TokenKind tokenKind, infix_parselet_t parselet) {
        auto found = infixParseletMap_.find(tokenKind);
        if(found != infixParseletMap_.end()) {
            ASSERT_FAIL("Specified tokenKind already exists in infixParseletMap_.")
        }
        infixParseletMap_[tokenKind] = parselet;
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
        }
        return token;
    }

    Token *consumeOptional(TokenKind tokenKind) {
        if(lexer_.peekToken()->kind() == tokenKind) {
            return lexer_.nextToken();
        }
        return nullptr;
    }

protected:
    PrattParser (Lexer &lexer, error::ErrorStream &errorStream) : errorStream_{errorStream}, lexer_{lexer} { }

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

        auto foundPrefixParselet = prefixParseletMap_.find(t->kind());
        if(foundPrefixParselet == prefixParseletMap_.end()) {
            errorStream_.error(
                error::ErrorKind::SurpriseToken,
                t->span(),
                message,
                t->text().c_str());

            throw ParseAbortedException();
        }

        ast::ExprStmt *left = foundPrefixParselet->second(t);

        t = lexer_.peekToken();
        if(t->kind() == TokenKind::END_OF_INPUT) {
            return left;
        }

        while(precedence < getOperatorPrecedence(t->kind())) {
            lexer_.nextToken();
            auto foundInfixParselet = infixParseletMap_.find(t->kind());
            if(foundInfixParselet == infixParseletMap_.end()) {
                errorStream_.error(
                    error::ErrorKind::SurpriseToken,
                    t->span(),
                    message,
                    t->text().c_str());

                throw ParseAbortedException();
            }

            left = foundInfixParselet->second(left, t);

            t = lexer_.peekToken();
            if(t->kind() == TokenKind::END_OF_INPUT) {
                return left;
            }
        }

        return left;
    }
};

}}}