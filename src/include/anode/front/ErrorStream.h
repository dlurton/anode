
#include "source.h"
#include "common/exception.h"
#include "common/string.h"
#include "front/ErrorKind.h"

#include "common/enum.h"
#include <ostream>
#include <memory>
#include <deque>

#pragma once

namespace anode { namespace front { namespace error {

struct ErrorDetail {
    ErrorKind errorKind;
    source::SourceSpan sourceSpan;

    ErrorDetail() : errorKind{ErrorKind::NoError}, sourceSpan{source::SourceSpan::Any} {}

    ErrorDetail(ErrorKind errorKind, source::SourceSpan sourceSpan) : errorKind{errorKind}, sourceSpan{sourceSpan} {}
};


class ErrorStream : no_assign, no_copy {
    std::ostream &output_;
    int errorCount_ = 0;
    int warningCount_ = 0;
    ErrorDetail firstError_;
    std::deque<std::string> contextMessageStack_;

    void showContext() {
        for(const auto &contextMessage : contextMessageStack_) {
             output_ << contextMessage << std::endl;
        }
    }

public:
    ErrorStream(std::ostream &output) : output_(output) { }

    void pushContextMessage(const std::string &message) {
        contextMessageStack_.push_back(message);
    }

    void popContextMessage() {
        ASSERT(contextMessageStack_.size());
        contextMessageStack_.pop_back();
    }

    int errorCount() { return errorCount_; }

    ErrorDetail &firstError() { return firstError_; }

    template<typename ... Args>
    void error(ErrorKind error, source::SourceSpan sourceSpan, const std::string &format, Args ... args) {
        if (errorCount_ == 0) {
            firstError_ = ErrorDetail(error, sourceSpan);
        }

        showContext();
        std::string message = string::format(format, args ...);
        output_ << sourceSpan.toString() << " error: " << message << std::endl;
        ++errorCount_;
    }

    template<typename ... Args>
    void warning(source::SourceSpan sourceSpan, const std::string &format, Args ... args) {
        std::string message = string::format(format, args ...);
        showContext();
        output_ << sourceSpan.toString() << " warning: " << message << std::endl;
        ++warningCount_;
    }
};

}}}