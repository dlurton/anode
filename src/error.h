
#include "source.h"
#include "exception.h"
#include "string.h"

#include <ostream>
#include <memory>

#pragma once

namespace lwnn {
    namespace error {
        class ErrorStream {
            std::ostream &output_;
            int errorCount_ = 0;
        public:
            ErrorStream(std::ostream &output) : output_(output) {  }
            int errorCount() { return errorCount_; }
            template<typename ... Args>
            void error(source::SourceSpan sourceSpan, const std::string& format, Args ... args) {
                output_ << sourceSpan.toString() << ": " << string::format(format, args ...) << std::endl;
                ++errorCount_;
            }
        };
    }
}