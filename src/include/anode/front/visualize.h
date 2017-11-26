
#pragma once

#include "ast.h"

namespace anode { namespace front {  namespace visualize {

void prettyPrint(ast::AstNode &module);

class IndentWriter {
    std::ostream &out_;
    int indent_ = 0;
    std::string indentString_;
    bool needsIndent_ = false;
public:
    IndentWriter(std::ostream &out, std::string indentString) : out_(out), indentString_(indentString) { }

    //Other "write" methods must all directly or indirectly call this in order for indentation to work
    void write(char c) {
        if(needsIndent_) {
            for(int i = 0; i < indent_; ++i) {
                out_ << indentString_;
            }
            needsIndent_ = false;
        }
        out_ << c;
        if(c == '\n') {
            needsIndent_ = true;
        }
    }

    void writeln(char c) {
        write(c);
        write('\n');
    }

    template<typename ... Args>
    void write( const std::string& format, Args ... args ) {
        std::string text = string::format(format, args ...);
        for(char c : text) {
            write(c);
        }
    }

    void writeln() {
        write('\n');
    }

    template<typename ... Args>
    void writeln( const std::string& format, Args ... args ) {
        write(format, args ...);
        write('\n');
    }


    void incIndent() { indent_++; }
    void decIndent() {
        indent_--;
        ASSERT(indent_ >= 0);
    }
};

}}}
