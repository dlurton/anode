
#pragma once
#include "common/enum.h"

namespace anode { namespace front { namespace error {

/**Every distinct type of compilation error is listed here.  It allows assertions that that the semantic checks under test are
 * failing for the expected reasons.
 */
BETTER_ENUM(ErrorKind, short,
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
);

}}}