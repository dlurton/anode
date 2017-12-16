
#pragma once
#include "common/enum.h"

namespace anode { namespace front { namespace error {

/**Every distinct type of compilation error is listed here.  It allows assertions that that the semantic checks under test are
 * failing for the expected reasons.
 */
BETTER_ENUM(
    ErrorKind,
    short,

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
    CannotNestTemplates,

    //Semantic errors
    InvalidImplicitCastInBinaryExpr,
    InvalidImplicitCastInIfCondition,
    InvalidImplicitCastInIfBodies,
    InvalidImplicitCastInInWhileCondition,
    InvalidImplicitCastInImplicitReturn,
    InvalidImplicitCastInFunctionCallArgument,
    InvalidImplicitCastInAssertCondition,
    SymbolAlreadyDefinedInScope,
    SymbolNotDefined,
    VariableUsedBeforeDefinition,
    InvalidExplicitCast,
    CannotAssignToLValue,
    SymbolIsNotAType,
    OperatorCannotBeUsedWithType,
    ExpressionIsNotFunction,
    LeftOfDotNotClass,
    ClassMemberNotFound,

    //Function related
    IncorrectNumberOfArguments,
    MethodNotDefined,

    //Template related
    SymbolIsNotATemplate,
    IncorrectNumberOfTemplateArguments,
    CircularTemplateReference,

    //Anonymous template related
    IncorrectNumberOfGenericArguments,
    TypeIsNotGenericButIsReferencedWithGenericArgs,
    GenericTypeWasNotExpandedWithSpecifiedArguments,
    OnlyClassesAndFunctionsAllowedInAnonymousTemplates,

    //Namespace related
    NamespaceDoesNotExist,
    IdentifierIsNotNamespace,
    ChildNamespaceDoesNotExist,
    MemberOfNamespaceIsNotNamespace,
    NamespaceMemberDoesNotExist
);

}}}