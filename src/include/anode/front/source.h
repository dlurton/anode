
#include "anode.h"

#pragma once

namespace anode { namespace source {
    /** Represents a location within a source file. */
    class SourceLocation : no_new {
        unsigned int line_;
        unsigned int position_;
    public:
        SourceLocation()
            : line_(-1), position_(-1) { }

        SourceLocation(std::size_t line, std::size_t position)
            : line_(line), position_(position) { }

        unsigned int line() const  { return line_; }
        unsigned int position() const { return position_; }

        std::string toString() {
            return string::format("%d:%d", line_, position_);
        }
    };
    bool operator== (SourceLocation a, SourceLocation b);
    bool operator!= (SourceLocation a, SourceLocation b);

    /** Represents a range within a source defined by a starting SourceLocation and and ending SourceLocation.*/
    class SourceSpan : public gc {
        /** The name of the input i.e. a filename, "stdin" or "REPL". */
        std::string name_;  //TODO:  I feel like it's horribly inefficient to be copying this around everywhere...
        SourceLocation start_;
        SourceLocation end_;
    public:
        SourceSpan(std::string sourceName, SourceLocation start, SourceLocation end)
            : name_(sourceName), start_(start), end_(end) { }

        SourceSpan(const SourceSpan &copyFrom)
            : name_(copyFrom.name_), start_(copyFrom.start_), end_(copyFrom.end_) { }

        std::string name() const { return name_;}
        SourceLocation start() const { return start_; }
        SourceLocation end() const { return end_; }

        std::string toString() {
            return string::format("%s:%s", name_.c_str(), start_.toString().c_str());
        }

        static SourceSpan Any;
    };

    bool operator==(SourceSpan &a, SourceSpan &b);
    bool operator!=(SourceSpan &a, SourceSpan &b);

}}