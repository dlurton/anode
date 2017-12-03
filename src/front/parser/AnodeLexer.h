
#pragma once

#include "common/containers.h"
#include "front/source.h"
#include "front/ErrorStream.h"

#include "Token.h"
#include "char.h"
#include "../SourceReader.h"

#include <vector>
#include <fstream>

using namespace anode::source;

namespace anode { namespace front { namespace parser {

inline bool isWhite(char_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool isDigit(char_t c) {
    return c >= '0' && c <= '9';
}

inline bool isLetter(char_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isValidInt(const string_t &str) {
    try {
        std::stoi(str);
        return true;
    } catch(std::out_of_range &) {
        return false;
    }
}

inline bool isValidFloat(const string_t &str) {
    try {
        std::stof(str);
        return true;
    } catch(std::out_of_range &) {
        return false;
    }
}

extern void InitStaticTokenLookup();

class AnodeLexer {

    SourceReader &reader_;
    error::ErrorStream &errorStream_;
    gc_ref_deque<Token> lookahead_;
    SourceLocation startLocation_;
public:
    NO_COPY_NO_ASSIGN(AnodeLexer)

    AnodeLexer(SourceReader &reader, error::ErrorStream &errorStream) : reader_(reader), errorStream_{errorStream} {
        InitStaticTokenLookup();
    }


    Token &nextToken() {
        if(!lookahead_.empty()) {
            Token &next = lookahead_.front();
            lookahead_.pop_front();
            return next;
        } else {
            return extractToken();
        }
    }

    Token &peekToken() {
        primeLookahead(1);
        return lookahead_.front();
    }

    bool eof() {
        return peekToken().kind() == TokenKind::END_OF_INPUT;
    }

    std::string inputName() {
        return reader_.inputName();
    }

private:

    void primeLookahead(size_t size) {
        while(lookahead_.size() < size) {
            lookahead_.push_back(extractToken());
        }
    }

    /** Discards all whitespace and comments if any are next. */
    void discardWhite();

    /** Discards the next whitespace character, if next.
     * @eturns true if any whitespace characters were discarded. */
    bool discardWhitespaceCharacters() {
        if(!isWhite(reader_.peek()))
            return false;

        do {
            reader_.next();
        } while(isWhite(reader_.peek()) && !reader_.eof());

        return true;
    }

    /** Discards a single-line comment if one is next.
     * @eturns true if a comment was discarded. */
    bool discardSingleLineComment();

    /** Discards a multi-line comment if one is next..
     * @eturns true if a comment was discarded. */
    bool discardMultilineComment();


    void markTokenStart() {
        startLocation_ = reader_.getCurrentSourceLocation();
    }

    SourceSpan getSourceSpanForCurrentToken() {
        return SourceSpan(reader_.inputName(), startLocation_, reader_.getCurrentSourceLocation());
    }

    Token &newToken(TokenKind kind, const string_t &text) {
        return *new Token(getSourceSpanForCurrentToken(), kind, text);
    }

    Token &newToken(TokenKind kind, const char *text) {
        return *new Token(getSourceSpanForCurrentToken(), kind, string_t(text));
    }

    Token &newToken(TokenKind kind, char_t c) {
        string_t str;
        str += c;
        return *new Token(getSourceSpanForCurrentToken(), kind, str);
    }

    Token &newUnexpectedToken(char_t c) {
        return newToken(TokenKind::UNEXPECTED, to_string(c));
    }

    Token &extractToken();
    Token &extractIdentifier();
    Token &extractLiteralNumber();
};

}}}