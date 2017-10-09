#pragma once

#include "AnodeLexer.h"
#include "PrattParser.h"
#include "front/ast.h"

#include <functional>

#include <fstream>
#include <sstream>

using namespace std::placeholders;

namespace anode { namespace front { namespace parser {

static source::SourceSpan getSourceSpan(const SourceSpan &start, const SourceSpan &end) {
    return source::SourceSpan(start.name(), start.start(), end.end());
}

class AnodeParser : public PrattParser<ast::ExprStmt> {
    gc_vector<ast::TemplateParameter*> templateParameters_;

    std::stack<scope::StorageKind> storageKindStack_;

    Token *consumeComma() {
        return consume(TokenKind::COMMA, ',');
    }

    Token *consumeEndOfStatment() {
        return consume(TokenKind::END_OF_STATEMENT, ';');
    }

    Token *consumeOpenParen() {
        return consume(TokenKind::OPEN_PAREN, '(');
    }

    Token *consumeOpenCurly() {
        return consume(TokenKind::OPEN_CURLY, '{');
    }

    Token *consumeCloseParen() {
        return consume(TokenKind::CLOSE_PAREN, ')');
    }

    Token *consumeGreaterThan()  {
        return consume(TokenKind::OP_GT, '>');
    }

    Token *consumeLessThan()  {
        return consume(TokenKind::OP_LT, '<');
    }

    Token *consumeColon() {
        return consume(TokenKind::OP_DEF, ':');
    }

    Token *consumeIdentifier() {
        return consume(TokenKind::ID, "identifier");
    }

    ast::ResolutionDeferredTypeRef *parseTypeRef() {
        gc_vector<ast::ResolutionDeferredTypeRef*> templateArgs;

        Token *token = consume(TokenKind::ID, "type identifier");
        Token *gt = consumeOptional(TokenKind::OP_LT);

        if(gt != nullptr) {
            do {
                templateArgs.push_back(parseTypeRef());
            } while (consume(TokenKind::OP_GT, TokenKind::COMMA, "'>' or ','")->kind() == TokenKind::COMMA);
        }

        return new ast::ResolutionDeferredTypeRef(token->span(), token->text(), templateArgs);
    }

    ast::ExprStmt *parseLiteralInt32(Token *token) {
        return new ast::LiteralInt32Expr(token->span(), token->intValue());
    }

    ast::ExprStmt *parseLiteralFloat(Token *token) {
        return new ast::LiteralFloatExpr(token->span(), token->floatValue());
    }

    ast::ExprStmt *parseLiteralBool(Token *token) {
        return new ast::LiteralBoolExpr(token->span(), token->boolValue());
    }

    ast::ExprStmt *parseVariableRef(Token *token) {
        if(consumeOptional(TokenKind::OP_DEF)) {
            ast::ResolutionDeferredTypeRef *typeRef = parseTypeRef();
            return new ast::VariableDeclExpr(
                getSourceSpan(token->span(), typeRef->sourceSpan()),
                token->text(),
                typeRef);

        }
        return new ast::VariableRefExpr(token->span(), token->text());
    }

    ast::ExprStmt *parseParensExpr(Token *) {
        ast::ExprStmt *expr = parseExpr();
        consumeCloseParen();
        return expr;
    }

    ast::ExprStmt *parsePrefixUnaryExpr(Token *operatorToken) {
        ast::UnaryOperationKind operationKind;
        switch(operatorToken->kind()) {
            case TokenKind::OP_NOT: operationKind = ast::UnaryOperationKind::Not; break;
            case TokenKind::OP_INC: operationKind = ast::UnaryOperationKind::PreIncrement; break;
            case TokenKind::OP_DEC: operationKind = ast::UnaryOperationKind::PreDecrement; break;
            default:
                ASSERT_FAIL("Unknown token kind for unary expression");
        }

        ast::ExprStmt *valueExpr = parseExpr();

        return new ast::UnaryExpr(
            getSourceSpan(operatorToken->span(), valueExpr->sourceSpan()),
            valueExpr,
            operationKind,
            operatorToken->span());
    }

    /** Fills the specified gc_vector with ExprStmt* until }.
     * @returns The } token. */
    Token *parseUntilCloseCurly(gc_vector<ast::ExprStmt*> &stmts) {
        Token *closeCurly;

        do {
            stmts.push_back(parseExpr());
        } while(!(closeCurly = consumeOptional(TokenKind::CLOSE_CURLY)));

        return closeCurly;
    }

    ast::ExprStmt *parseExpressionList(Token *openCurly) {
        gc_vector<ast::ExprStmt*> stmts;
        Token *closeCurly = parseUntilCloseCurly(stmts);
        return new ast::ExpressionList(getSourceSpan(openCurly->span(), closeCurly->span()), stmts);
    }
    ast::ExprStmt *parseCompoundStmt(Token *openCurly) {
        gc_vector<ast::ExprStmt*> stmts;
        Token *closeCurly = parseUntilCloseCurly(stmts);
        return new ast::CompoundExpr(getSourceSpan(openCurly->span(), closeCurly->span()), storageKindStack_.top(), stmts);
    }

    ast::ExprStmt *parseCastExpr(Token *castKeyword) {
        consumeLessThan();
        ast::ResolutionDeferredTypeRef *typeRef = parseTypeRef();
        consumeGreaterThan();
        consumeOpenParen();
        ast::ExprStmt *valueExpr = parseExpr();
        Token *closeParen = consumeCloseParen();

        return new ast::CastExpr(
            getSourceSpan(castKeyword->span(), closeParen->span()),
            typeRef,
            valueExpr,
            ast::CastKind::Explicit);
    }

    ast::ExprStmt *parseNewExpr(Token *newKeyword) {
        ast::ResolutionDeferredTypeRef *typeRef = parseTypeRef();
        consumeOpenParen();
        Token *closeParen = consumeCloseParen();

        return new ast::NewExpr(
            getSourceSpan(newKeyword->span(), closeParen->span()),
            typeRef);
    }

    ast::ExprStmt *parseConditional(Token *openingOp) {
        ast::ExprStmt *condExpr = parseExpr();
        consumeEndOfStatment();
        ast::ExprStmt *thenExpr = parseExpr();
        consumeEndOfStatment();
        ast::ExprStmt *elseExpr = parseExpr();
        Token *closingParen = consumeCloseParen();

        return new ast::IfExprStmt(
            getSourceSpan(openingOp->span(), closingParen->span()),
            condExpr,
            thenExpr,
            elseExpr
        );
    }

    ast::ExprStmt *parseIfExpr(Token *ifKeyword) {
        consumeOpenParen();
        ast::ExprStmt *condExpr = parseExpr();
        consumeCloseParen();

        ast::ExprStmt *thenExpr = parseExpr();
        ast::ExprStmt *elseExpr = nullptr;
        if(consumeOptional(TokenKind::KW_ELSE)) {
            elseExpr = parseExpr();
        }

        return new ast::IfExprStmt(
            getSourceSpan(ifKeyword->span(), thenExpr->sourceSpan()),
            condExpr,
            thenExpr,
            elseExpr
        );
    }

    ast::ExprStmt *parseWhile(Token *whileKeyword) {
        consumeOpenParen();
        ast::ExprStmt *condExpr = parseExpr();
        consumeCloseParen();
        ast::ExprStmt *bodyExpr = parseExpr();

        return new ast::WhileExpr(
            getSourceSpan(whileKeyword->span(), bodyExpr->sourceSpan()),
            condExpr,
            bodyExpr
        );
    }

    /** retval.first is parsed list of argument exprs, retval.second is the closing ')' */
    std::pair<gc_vector<ast::ExprStmt*>, Token*> parseFuncCallArguments() {
        gc_vector<ast::ExprStmt*> arguments;
        Token *closeParen = consumeOptional(TokenKind::CLOSE_PAREN);
        //If argument list is not empty
        if(!closeParen) {
            do {
                ast::ExprStmt *argExpr = parseExpr();
                arguments.push_back(argExpr);
                closeParen = consume(TokenKind::COMMA, TokenKind::CLOSE_PAREN, ',', ')');
            } while(closeParen->kind() != TokenKind::CLOSE_PAREN);
        }
        return std::pair<gc_vector<ast::ExprStmt*>, Token*>(arguments, closeParen);
    };

    ast::ExprStmt *parseFuncCallExpr(ast::ExprStmt *funcExpr, Token *openParen) {
        auto argsAndCloseParen = parseFuncCallArguments();


        return new ast::FuncCallExpr(
            getSourceSpan(funcExpr->sourceSpan(), argsAndCloseParen.second->span()),
            nullptr, //No instanceExpr
            openParen->span(),
            funcExpr,
            argsAndCloseParen.first
        );
    }

    ast::ExprStmt *parseFuncDef(Token *funcKeyword) {
        Token *identifier = consumeIdentifier();
        consumeColon();
        ast::ResolutionDeferredTypeRef *returnTypeRef = parseTypeRef();
        consumeOpenParen();

        gc_vector<ast::ParameterDef*> parameters;

        Token *closeParen = consumeOptional(TokenKind::CLOSE_PAREN);
        //If parameter list is not empty
        if(!closeParen) {
            do {
                Token *name = consumeIdentifier();
                consumeColon();
                ast::ResolutionDeferredTypeRef *parameterTypeRef = parseTypeRef();
                parameters.push_back(
                    new ast::ParameterDef(
                        getSourceSpan(name->span(), parameterTypeRef->sourceSpan()),
                        name->text(),
                        parameterTypeRef
                    )
                );

                closeParen = consume(TokenKind::COMMA, TokenKind::CLOSE_PAREN, ',', ')');
            } while(closeParen->kind() != TokenKind::CLOSE_PAREN);
        }

        storageKindStack_.push(scope::StorageKind::Local);
        ast::ExprStmt *funcBody = parseExpr();
        storageKindStack_.pop();

        return new ast::FuncDefStmt(
            getSourceSpan(funcKeyword->span(), funcBody->sourceSpan()),
            identifier->text(),
            returnTypeRef,
            parameters,
            funcBody
        );
    }

    ast::ExprStmt *parseClassDef(Token *classKeyword) {
        Token *className = consumeIdentifier();
        storageKindStack_.push(scope::StorageKind::Instance);

        ast::ExprStmt *classBody = parseExpr();

        auto *compoundExpr = dynamic_cast<ast::CompoundExpr*>(classBody);
        if(!compoundExpr) {
            gc_vector<ast::ExprStmt*> stmts;
            stmts.push_back(classBody);
            compoundExpr = new ast::CompoundExpr(classBody->sourceSpan(), scope::StorageKind::Instance, stmts);
        } else {
            compoundExpr->scope()->setStorageKind(scope::StorageKind::Instance);
        }

        storageKindStack_.pop();
        compoundExpr->scope()->setName(className->text());

        if(templateParameters_.empty()) {
            return new ast::CompleteClassDefinition(
                getSourceSpan(classKeyword->span(), classBody->sourceSpan()),
                className->text(),
                compoundExpr
            );
        } else {
            return new ast::GenericClassDefinition(
                getSourceSpan(classKeyword->span(), classBody->sourceSpan()),
                className->text(),
                deepCopyVector(templateParameters_),
                compoundExpr
            );
        }

    }

    ast::ExprStmt *parseAssert(Token *assertKeyword) {
        consumeOpenParen();
        ast::ExprStmt *cond = parseExpr();
        Token *closeParen = consumeCloseParen();

        return new ast::AssertExprStmt(
            getSourceSpan(assertKeyword->span(), closeParen->span()),
            cond);
    }

    ast::ExprStmt *parseTemplate(Token *templateKeyword) {
        Token *templateName = consumeIdentifier();
        gc_vector<ast::TemplateParameter*> parameters;

        //TODO:  error when templates are nested or adjust to allow for nested templates
        consumeOpenParen();
        while(lexer_.peekToken()->kind() != TokenKind::CLOSE_PAREN) {
            Token *identifier = consumeIdentifier();
            parameters.push_back(new ast::TemplateParameter(identifier->span(), identifier->text()));
            //TODO: allows trailing ,  Bad?
            consumeOptional(TokenKind::COMMA);
        }
        consumeCloseParen();

        templateParameters_ = parameters;
        auto *body = upcast<ast::ExpressionList>(parseExpressionList(consumeOpenCurly()));
        templateParameters_.clear();

        return new ast::TemplateExprStmt(
            getSourceSpan(templateKeyword->span(), body->sourceSpan()),
            templateName->text(),
            parameters,
            body);
    }

    ast::ExprStmt *parseExpand(Token *expandKeyword) {
        gc_vector<ast::TypeRef*> templateArgs;
        Token *templateName = consumeIdentifier();
        Token *terminator = nullptr;
        consumeOpenParen();

        do {
            templateArgs.push_back(parseTypeRef());
        } while ((terminator = consume(TokenKind::CLOSE_PAREN, TokenKind::COMMA, "'>' or ','"))->kind() == TokenKind::COMMA);

        return new ast::TemplateExpansionExprStmt(
            getSourceSpan(expandKeyword->span(), terminator->span()),
            templateName->text(), templateArgs);
    }

    ast::ExprStmt *parseDotExpr(ast::ExprStmt *lValue, Token *operatorToken) {
        Token *memberName = consumeIdentifier();

        if(Token *openParen = consumeOptional(TokenKind::OPEN_PAREN)) {
            auto argsAndCloseParen = parseFuncCallArguments();

            return new ast::FuncCallExpr(
                getSourceSpan(lValue->sourceSpan(), argsAndCloseParen.second->span()),
                lValue,
                openParen->span(),
                new ast::MethodRefExpr(memberName->span(), memberName->text()),
                argsAndCloseParen.first
            );
        }

        return new ast::DotExpr(
            getSourceSpan(lValue->sourceSpan(), memberName->span()),
            operatorToken->span(),
            lValue,
            memberName->text()
        );
    }

    ast::ExprStmt *parseBinaryExpr(ast::ExprStmt *lValue, Token *operatorToken);

    Associativity getOperatorAssociativity(TokenKind kind);
    int getOperatorPrecedence(TokenKind kind) override;

public:

    AnodeParser(AnodeLexer &lexer, error::ErrorStream &errorStream) : PrattParser(lexer, errorStream) {
        //Prefix parselets
        registerGenericParselet(TokenKind::LIT_INT, std::bind(&AnodeParser::parseLiteralInt32, this, _1));
        registerGenericParselet(TokenKind::LIT_FLOAT, std::bind(&AnodeParser::parseLiteralFloat, this, _1));
        registerGenericParselet(TokenKind::KW_TRUE, std::bind(&AnodeParser::parseLiteralBool, this, _1));
        registerGenericParselet(TokenKind::KW_FALSE, std::bind(&AnodeParser::parseLiteralBool, this, _1));
        registerGenericParselet(TokenKind::ID, std::bind(&AnodeParser::parseVariableRef, this, _1));

        registerGenericParselet(TokenKind::OP_NOT, std::bind(&AnodeParser::parsePrefixUnaryExpr, this, _1));
        registerGenericParselet(TokenKind::OP_INC, std::bind(&AnodeParser::parsePrefixUnaryExpr, this, _1));
        registerGenericParselet(TokenKind::OP_DEC, std::bind(&AnodeParser::parsePrefixUnaryExpr, this, _1));

        registerGenericParselet(TokenKind::OPEN_CURLY, std::bind(&AnodeParser::parseCompoundStmt, this, _1));
        registerGenericParselet(TokenKind::OPEN_PAREN, std::bind(&AnodeParser::parseParensExpr, this, _1));
        registerGenericParselet(TokenKind::KW_CAST, std::bind(&AnodeParser::parseCastExpr, this, _1));
        registerGenericParselet(TokenKind::KW_NEW, std::bind(&AnodeParser::parseNewExpr, this, _1));
        registerGenericParselet(TokenKind::OP_COND, std::bind(&AnodeParser::parseConditional, this, _1));
        registerGenericParselet(TokenKind::KW_IF, std::bind(&AnodeParser::parseIfExpr, this, _1));
        registerGenericParselet(TokenKind::KW_WHILE, std::bind(&AnodeParser::parseWhile, this, _1));
        registerGenericParselet(TokenKind::KW_FUNC, std::bind(&AnodeParser::parseFuncDef, this, _1));
        registerGenericParselet(TokenKind::KW_CLASS, std::bind(&AnodeParser::parseClassDef, this, _1));
        registerGenericParselet(TokenKind::KW_ASSERT, std::bind(&AnodeParser::parseAssert, this, _1));
        registerGenericParselet(TokenKind::KW_TEMPLATE, std::bind(&AnodeParser::parseTemplate, this, _1));
        registerGenericParselet(TokenKind::KW_EXPAND, std::bind(&AnodeParser::parseExpand, this, _1));

        //Infix parselets
        registerInfixParselet(TokenKind::OP_ADD, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_SUB, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_MUL, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_DIV, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_EQ, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_NEQ, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_GT, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_GTE, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_LT, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_LTE, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_LAND, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_LOR, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_ASSIGN, std::bind(&AnodeParser::parseBinaryExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OP_DOT, std::bind(&AnodeParser::parseDotExpr, this, _1, _2));
        registerInfixParselet(TokenKind::OPEN_PAREN, std::bind(&AnodeParser::parseFuncCallExpr, this, _1, _2));
    }

    ast::Module *parseModule() {
        storageKindStack_.push(scope::StorageKind::Global);
        gc_vector<ast::ExprStmt*> exprs;

        while(!lexer_.eof()) {
            exprs.push_back(parseExpr());
        }

        ast::CompoundExpr *body = new ast::CompoundExpr(source::SourceSpan::Any, scope::StorageKind::Global, exprs);
        body->scope()->setName(lexer_.inputName());

        ASSERT(storageKindStack_.size() == 1);
        storageKindStack_.pop();

        return new ast::Module(lexer_.inputName(), body);
    }
};

}}}