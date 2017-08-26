
#include "common/string_format.h"

#pragma once

namespace lwnn {
    namespace source {
        /** Represents a location within a source file. */
        class SourceLocation {
            std::size_t line_;
            std::size_t position_;
        public:
            SourceLocation(std::size_t line, std::size_t position)
                : line_(line), position_(position) { }

            std::size_t line() const  { return line_; }
            std::size_t position() const { return line_; }

            std::string toString() {
                return string::format("(%d, %d)", line_, position_);
            }
        };
        bool operator== (SourceLocation a, SourceLocation b);
        bool operator!= (SourceLocation a, SourceLocation b);

        /** Represents a range within a source defined by a starting SourceLocation and and ending SourceLocation.*/
        class SourceSpan {
            /** The name of the input i.e. a filename, "stdin" or "REPL". */
            std::string name_;  //TODO:  I feel like it's horribly inefficient to be copying this around everywhere...
            SourceLocation start_;
            SourceLocation end_;
        public:
            SourceSpan(std::string sourceName, SourceLocation start, SourceLocation end)
                : name_(sourceName), start_(start), end_(end) { }

            SourceSpan(const SourceSpan &copyFrom)
                : name_(copyFrom.name_), start_(copyFrom.start_), end_(copyFrom.end_) { }

            std::string const name() { return name_;}
            SourceLocation const start() { return start_; }
            SourceLocation const end() { return end_; }

            std::string toString() {
                return string::format("%s %s", name_.c_str(), start_.toString().c_str());
            }

            static SourceSpan Any;
        };
        bool operator==(SourceSpan &a, SourceSpan &b);
        bool operator!=(SourceSpan &a, SourceSpan &b);
    }
}