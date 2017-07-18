/* Ahttps://stackoverflow.com/questions/29971097/how-to-create-ast-with-antlr4 */

grammar Lwnn;

module
    : (statements_=statement ';')* EOF
    ;

statement
    : varDecl
    | expr
    ;

expr
    :   '(' expr ')'                        # parensExpr
//  |   op=('+'|'-') expr                   # unaryExpr
    |   left=expr op=('*'|'/') right=expr   # binaryExpr
    |   left=expr op=('+'|'-') right=expr   # binaryExpr
//  |   func=ID '(' expr ')'                # funcExpr
    |   value=LIT_INT                       # literalInt32Expr
    |   value=LIT_FLOAT                     # literalFloatExpr
    |   var=ID                              # variableRefExpr
    ;

varDecl
    : name=ID ':' type=ID
    | name=ID ':' type=ID '=' initializer=expr
    ;

OP_ADD: '+';
OP_SUB: '-';
OP_MUL: '*';
OP_DIV: '/';

//NUM :   [0-9]+ ('.' [0-9]+)? ([eE] [+-]? [0-9]+)?;
LIT_INT:    [0-9]+;
LIT_FLOAT:  [0-9]+'.'[0-9]+;
ID:         [a-zA-Z]+;
WS:         [ \t\r\n] -> channel(HIDDEN);
