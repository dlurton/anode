
#pragma once

#include "parser/char.h"
#include "anode.h"

#include <deque>

namespace anode { namespace front { namespace parser {

/** This is a kind of "stream" which is used by the lexer to read characters.
 * It can lookahead an unlimited number of characters and also keeps track of the current line and character index.
 */
class SourceReader : no_new, no_copy, no_assign {
    string_t inputName_;
    std::istream &inputStream_;
    int lineNo_;
    size_t charPositionInLine_ = 1;
    std::deque<char_t> lookahead_;

    char_t readChar() {
        int c = inputStream_.get();
        return static_cast<char_t>(c);
    }

    void primeLookahead(size_t toSize) {
        for(size_t i = lookahead_.size(); i < toSize; ++i) {
            lookahead_.push_back(readChar());
        }
    }

public:
    SourceReader(const string_t &inputName_, std::istream &inputStream, int lineNumOffset = 1)
        : inputName_(inputName_), inputStream_(inputStream), lineNo_{lineNumOffset} {  }

    string_t inputName() { return inputName_; }

    bool eof() { return (lookahead_.size() == 0 || lookahead_.front() == -1) && inputStream_.eof(); }

    source::SourceLocation getCurrentSourceLocation() {
        return source::SourceLocation(lineNo_, charPositionInLine_);
    }


    /** Checks if the characters in the candidate string match the next characters in the input stream.
     * If so, consume the matching characters from the input stream and return true;
     */
    bool match(const string_t &candidate) {

        if(!peekMatch(candidate))
            return false;

        for(size_t i = 0; i < candidate.size(); ++i) {
            next();
        }

        return true;
    }

    bool peekMatch(const string_t &candidate) {
        primeLookahead(candidate.size());

        for(size_t i = 0; i < candidate.size(); ++i) {
            if(lookahead_[i] != candidate[i]) {
                return false;
            }
        }
        return true;
    }

    char_t peek() {
        primeLookahead(1);
        return lookahead_.front();
    }

    char_t peek(size_t n) {
        primeLookahead(n + 1);
        return lookahead_[n];
    }

    char_t next() {
        char_t c;

        if(lookahead_.size() > 0) {
            c = lookahead_.front();
            lookahead_.pop_front();
        } else {
            c = (char_t)inputStream_.get();
        }

        if(c == '\n') {
            ++lineNo_;
            charPositionInLine_ = 1;
        } else {
            ++charPositionInLine_;
        }

        return c;
    }
};

}}}