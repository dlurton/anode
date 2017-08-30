

#pragma once

#include "front/source.h"
#include "char.h"

namespace lwnn { namespace front { namespace parser {


//The only reason this is an unsigned short and not an unsigned char is because catch.hpp will try to give me literal characters
//when it shows the expansion in the case of a failure.
enum class TokenKind : unsigned short {
    NotSet,
    END_OF_INPUT,
    UNEXPECTED,
    END_OF_STATEMENT,
    OP_NOT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_ASSIGN,
    OP_EQ,
    OP_NEQ,
    OP_GT,
    OP_LT,
    OP_LTE,
    OP_GTE,
    OP_LAND,    //Logical And
    OP_LOR,     //Logical Or
    OP_DOT,
    OP_DEF,
    OP_INC,
    OP_DEC,
    OP_COND,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_CURLY,
    CLOSE_CURLY,
    ID,
    LIT_INT,
    LIT_FLOAT,
    KW_TRUE,
    KW_FALSE,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FUNC,
    KW_CAST,
    KW_CLASS,
    KW_ASSERT,
    COMMA,
};

class Token : public gc {
    const source::SourceSpan span_;
    const TokenKind kind_;
    const string_t text_;
public:
    Token(source::SourceSpan span, TokenKind kind, string_t text) : span_{span}, kind_{kind}, text_{text} { }

    const source::SourceSpan &span() const { return span_; }

    TokenKind kind() const { return kind_; }

    string_t text() const { return text_; }

    int intValue() { return std::stoi(text_); }
    float floatValue() { return std::stof(text_); }
    bool boolValue() { return text_.front() == 't'; }
};


}}}