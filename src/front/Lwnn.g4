/* Ahttps://stackoverflow.com/questions/29971097/how-to-create-ast-with-antlr4 */

grammar Lwnn;

module
    : (statements_=statement)* EOF
    ;

// continue following this:  https://github.com/antlr/grammars-v4/blob/master/java/Java.g4#L407
statement
    : expr ';'                                                                 # exprStmt
    | compoundExprStmt                                                         # compoundStmt
    | KW_IF '(' cond=expr ')' thenStmt=statement ('else' elseStmt=statement)?  # ifStmt
    ;

// Generally, we are following this for operator precdence:
// http://en.cppreference.com/w/cpp/language/operator_precedence
expr
    : name=ID ':' type=ID                                             # varDeclExpr
    | '(' expr ')'                                                    # parensExpr
//  | op=('+'|'-') expr                                               # unaryExpr
    | left=expr op=(OP_MUL | OP_DIV) right=expr                       # binaryExpr
    | left=expr op=(OP_ADD | OP_SUB) right=expr                       # binaryExpr
    | left=expr op=(OP_GT | OP_GTE | OP_LT | OP_LTE) right=expr       # binaryExpr
    | left=expr op=(OP_EQ | OP_NEQ) right=expr                        # binaryExpr
    | left=expr op=OP_AND right=expr                                  # binaryExpr
    | left=expr op=OP_OR right=expr                                   # binaryExpr
    | <assoc=right> left=expr op=OP_ASSIGN right=expr                 # binaryExpr
//  | func=ID '(' expr ')'                                            # funcExpr
    | value=LIT_INT                                                   # literalInt32Expr
    | value=LIT_FLOAT                                                 # literalFloatExpr
    | value=litBool                                                   # literalBool
    | var=ID                                                          # variableRefExpr
    | 'cast' OP_LT type=ID OP_GT '(' expr ')'                         # castExpr
    | '(?' cond=expr ',' thenExpr=expr ',' elseExpr=expr ')'          # ternaryExpr
    | compoundExprStmt                                                # compoundExpr
    | KW_IF '(' cond=expr ')' thenExpr=expr ('else' elseExpr=expr)?   # ifExpr
    ;

//compoundExprStmt can be an expression or a statement, depending on the context
compoundExprStmt
    : '{' (stmts=statement )* '}'
    ;

litBool
    : KW_TRUE
    | KW_FALSE;

OP_ADD: '+';
OP_SUB: '-';
OP_MUL: '*';
OP_DIV: '/';
OP_ASSIGN: '=';
OP_EQ: '==';
OP_NEQ: '!=';
OP_OR: '||';
OP_AND: '&&';
OP_GT: '>';
OP_LT: '<';
OP_GTE: '>=';
OP_LTE: '<=';

KW_TRUE: 'true';
KW_FALSE: 'false';
KW_IF: 'if';

//NUM :   [0-9]+ ('.' [0-9]+)? ([eE] [+-]? [0-9]+)?;
LIT_INT:    '-'?[0-9]+;
LIT_FLOAT:  '-'?[0-9]+'.'[0-9]+;
ID:         [a-zA-Z_][a-zA-Z0-9_]*;
WS:         [ \t\r\n] -> channel(HIDDEN);
