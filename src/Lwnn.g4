/* Ahttps://stackoverflow.com/questions/29971097/how-to-create-ast-with-antlr4 */

grammar Lwnn;

compileUnit
    :   expr EOF
    ;

expr
    :   '(' expr ')'                        # parensExpr
//  |   op=('+'|'-') expr                   # unaryExpr
    |   left=expr op=('*'|'/') right=expr   # infixExpr
    |   left=expr op=('+'|'-') right=expr   # infixExpr
//  |   func=ID '(' expr ')'                # funcExpr
    |   value=NUM                           # numberExpr
    |   var=ID                              # varRefExpr
    ;

varDecl
    : name=ID ':' type=ID;

TYPE_REF
    : KW_FLOAT
    | KW_INT;

KW_INT: 'int';
KW_FLOAT: 'float';

OP_ADD: '+';
OP_SUB: '-';
OP_MUL: '*';
OP_DIV: '/';

//NUM :   [0-9]+ ('.' [0-9]+)? ([eE] [+-]? [0-9]+)?;
NUM :   [0-9]+;
ID  :   [a-zA-Z]+;
WS  :   [ \t\r\n] -> channel(HIDDEN);
