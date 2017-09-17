
#include "source.h"
#include "common/exception.h"
#include "common/string_format.h"

#include <ostream>
#include <memory>

#pragma once

namespace anode { namespace error {

/**Every distinct type of compilation error is listed here.  Currently, this is mainly used for integration testing.  It
 * allows assertions that that the semantic checks under test are indeed failing.  Potentially, we may also want to
 * use this as a basis for numbering the errors.
 */
enum class ErrorKind : unsigned short {
    NoError,
    //Lexer errors
    UnexpectedCharacter,
    InvalidLiteralInt32,
    InvalidLiteralFloat,
    UnexpectedEofInMultilineComment,

    //Parser errors
    UnexpectedToken,
    Syntax,
    SurpriseToken,

    //Semantic errors
    InvalidImplicitCastInBinaryExpr,
    InvalidImplicitCastInIfCondition,
    InvalidImplicitCastInIfBodies,
    InvalidImplicitCastInInWhileCondition,
    InvalidImplicitCastInImplicitReturn,
    InvalidImplicitCastInFunctionCallArgument,
    InvalidImplicitCastInAssertCondition,
    SymbolAlreadyDefinedInScope,
    VariableNotDefined,
    VariableUsedBeforeDefinition,
    TypeNotDefined,
    InvalidExplicitCast,
    CannotAssignToLValue,
    SymbolIsNotAType,
    OperatorCannotBeUsedWithType,
    LeftOfDotNotClass,
    ClassMemberNotFound,
    IncorrectNumberOfArguments,
    MethodNotDefined
};

class ErrorStream {
    std::ostream &output_;
    int errorCount_ = 0;
    int warningCount_ = 0;
    ErrorKind firstError_ = ErrorKind::NoError;
public:
    ErrorStream(std::ostream &output) : output_(output) {  }
    int errorCount() { return errorCount_; }

    ErrorKind firstError() { return firstError_; }

    template<typename ... Args>
    void error(ErrorKind error, source::SourceSpan sourceSpan, const std::string& format, Args ... args) {
        if(errorCount_ == 0) {
            firstError_ = error;
        }
        output_ << sourceSpan.toString() << " error: " << string::format(format, args ...) << std::endl;
        ++errorCount_;
    }

    template<typename ... Args>
    void warning(source::SourceSpan sourceSpan, const std::string& format, Args ... args) {
        output_ << sourceSpan.toString() << " warning: " << string::format(format, args ...) << std::endl;
        ++warningCount_;
    }
};

}}