/* Ahttps://stackoverflow.com/questions/29971097/how-to-create-ast-with-antlr4 */

grammar Lwnn;

module
    : (statements_=stmt)* EOF
    ;

stmt
    : exprStmt          # expressionStatement
    | classDef          # classDefinition
    ;

 exprStmt
    : expr ';'                                                                 # simpleExpr
    | compoundExprStmt                                                         # compoundStmt
    | KW_IF '(' cond=expr ')' thenStmt=exprStmt ('else' elseStmt=exprStmt)?    # ifStmt
    | KW_WHILE '(' cond=expr ')' body=expr ';'                                 # whileStmt
    | KW_WHILE '(' cond=expr ')' body=compoundExprStmt                         # whileStmtCompound
    ;

// Generally, we are following this for operator precdence:
// http://en.cppreference.com/w/cpp/language/operator_precedence
expr
    : name=ID ':' type=typeRef                                        # varDeclExpr
    | '(' expr ')'                                                    # parensExpr
//  | op=('+'|'-') expr                                               # unaryExpr
    | left=expr op=(OP_MUL | OP_DIV) right=expr                       # binaryExpr
    | left=expr op=(OP_ADD | OP_SUB) right=expr                       # binaryExpr
    | left=expr op=(OP_GT | OP_GTE | OP_LT | OP_LTE) right=expr       # binaryExpr
    | left=expr op=(OP_EQ | OP_NEQ) right=expr                        # binaryExpr
    | left=expr op=OP_AND right=expr                                  # binaryExpr
    | left=expr op=OP_OR right=expr                                   # binaryExpr
    | left=expr op=OP_DOT right=ID                                    # dotExpr
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
//    | KW_WHILE '(' cond=expr ')' body=expr                            # whileExpr
    ;

//compoundExprStmt can be an expression or a statement, depending on the context
compoundExprStmt
    : '{' (exprStmt)* '}'
    ;

classDef
    : 'class' name=ID classBody
    ;

classBody
    : '{' statements=stmt* '}'
    ;

typeRef
    : ID //Note (this will one day be more complicated than just a single identifier.
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
OP_DOT: '.';

KW_TRUE: 'true';
KW_FALSE: 'false';
KW_IF: 'if';
KW_WHILE: 'while';

//NUM :   [0-9]+ ('.' [0-9]+)? ([eE] [+-]? [0-9]+)?;
LIT_INT:    '-'?[0-9]+;
LIT_FLOAT:  '-'?[0-9]+'.'[0-9]+;
ID:         [a-zA-Z_][a-zA-Z0-9_]*;
WS:         [ \t\r\n] -> channel(HIDDEN);
