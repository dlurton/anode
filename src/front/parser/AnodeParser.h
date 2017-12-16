#pragma once

#include "AnodeLexer.h"
#include "PrattParser.h"
#include "front/ast.h"

#include <functional>
#include <fstream>
#include <sstream>

using namespace std::placeholders;

namespace anode { namespace front { namespace parser {


class AnodeParser : public PrattParser<ast::ExprStmt> {

    gc_ref_vector<ast::TemplateParameter> templateParameters_;
    std::stack<scope::StorageKind> storageKindStack_;

    bool parsingTemplate_ = false;
    bool parsingAnonymousTemplate_ = false;

    inline static ast::Identifier makeIdentifier(Token &t) {
        return ast::Identifier(t.span(), t.text());
    }

    inline static source::SourceSpan makeSourceSpan(const SourceSpan &start, const SourceSpan &end) {
        return source::SourceSpan(start.name(), start.start(), end.end());
    }

    Token &consumeComma() {
        return consume(TokenKind::COMMA, ',');
    }

    Token &consumeEndOfStatment() {
        return consume(TokenKind::END_OF_STATEMENT, ';');
    }

    Token &consumeOpenParen() {
        return consume(TokenKind::OPEN_PAREN, '(');
    }

    Token &consumeOpenCurly() {
        return consume(TokenKind::OPEN_CURLY, '{');
    }
    Token &consumeOpGt() {
        return consume(TokenKind::OP_GT, '>');
    }

    Token &consumeOpLt() {
        return consume(TokenKind::OP_LT, '<');
    }

    Token &consumeCloseParen() {
        return consume(TokenKind::CLOSE_PAREN, ')');
    }

    Token &consumeGreaterThan()  {
        return consume(TokenKind::OP_GT, '>');
    }

    Token &consumeLessThan()  {
        return consume(TokenKind::OP_LT, '<');
    }

    Token &consumeColon() {
        return consume(TokenKind::OP_DEF, ':');
    }

    ast::Identifier *consumeOptionalIdentifier() {
         Token *id = consumeOptional(TokenKind::ID);
         if(id != nullptr) {
            return new ast::Identifier(id->span(), id->text());
        }
        return nullptr;
    }

    ast::Identifier parseIdentifier() {
        return makeIdentifier(consume(TokenKind::ID, "identifier"));
    }

    ast::MultiPartIdentifier parseQualifiedIdentifier() {
        std::vector<ast::Identifier> parts;
        parts.emplace_back(parseIdentifier());
        while(consumeOptional(TokenKind::OP_NAMESPACE)) {
            parts.emplace_back(parseIdentifier());
        }

        return ast::MultiPartIdentifier(parts);
    }

    ast::MultiPartIdentifier parseQualifiedIdentifier(Token &first) {
        ASSERT(first.kind() == TokenKind::ID);
        std::vector<ast::Identifier> parts;
        parts.emplace_back(makeIdentifier(first));
        while(consumeOptional(TokenKind::OP_NAMESPACE)) {
            parts.emplace_back(parseIdentifier());
        }

        return ast::MultiPartIdentifier(parts);
    }

    ast::ResolutionDeferredTypeRef &parseTypeRef() {
        gc_ref_vector<ast::ResolutionDeferredTypeRef> templateArgs;

        ast::MultiPartIdentifier typeId = parseQualifiedIdentifier();
        Token *gt = consumeOptional(TokenKind::OP_LT);

        if(gt != nullptr) {
            do {
                templateArgs.emplace_back(parseTypeRef());
            } while (consume(TokenKind::OP_GT, TokenKind::COMMA, "'>' or ','").kind() == TokenKind::COMMA);
        }

        return *new ast::ResolutionDeferredTypeRef(typeId.span(), typeId, templateArgs);
    }

    ast::ExprStmt &parseLiteralInt32(Token &token) {
        return *new ast::LiteralInt32Expr(token.span(), token.intValue());
    }

    ast::ExprStmt &parseLiteralFloat(Token &token) {
        return *new ast::LiteralFloatExpr(token.span(), token.floatValue());
    }

    ast::ExprStmt &parseLiteralBool(Token &token) {
        return *new ast::LiteralBoolExpr(token.span(), token.boolValue());
    }

    ast::ExprStmt &parseVariableRef(Token &firstPart) {
        ast::MultiPartIdentifier identifier = parseQualifiedIdentifier(firstPart);
        if(consumeOptional(TokenKind::OP_DEF)) {
            ast::ResolutionDeferredTypeRef &typeRef = parseTypeRef();
            return *new ast::VariableDeclExpr(
                makeSourceSpan(firstPart.span(), typeRef.sourceSpan()),
                identifier,
                typeRef);

        }
        return *new ast::VariableRefExpr(firstPart.span(), identifier);
    }

    ast::ExprStmt &parseParensExpr(Token &) {
        ast::ExprStmt &expr = parseExpr();
        consumeCloseParen();
        return expr;
    }

    ast::ExprStmt &parsePrefixUnaryExpr(Token &operatorToken) {
        ast::UnaryOperationKind operationKind;
        switch(operatorToken.kind()) {
            case TokenKind::OP_NOT: operationKind = ast::UnaryOperationKind::Not; break;
            case TokenKind::OP_INC: operationKind = ast::UnaryOperationKind::PreIncrement; break;
            case TokenKind::OP_DEC: operationKind = ast::UnaryOperationKind::PreDecrement; break;
            default:
                ASSERT_FAIL("Unknown token kind for unary expression");
        }

        ast::ExprStmt &valueExpr = parseExpr();

        return *new ast::UnaryExpr(
            makeSourceSpan(operatorToken.span(), valueExpr.sourceSpan()),
            valueExpr,
            operationKind,
            operatorToken.span());
    }

    /** Fills the specified gc_vector with ExprStmt* until }.
     * @returns The } token. */
    Token &parseUntilCloseCurly(gc_ref_vector<ast::ExprStmt> &stmts) {
        Token *closeCurly;

        while(!(closeCurly = consumeOptional(TokenKind::CLOSE_CURLY))) {
            stmts.emplace_back(parseExpr());
        }

        return *closeCurly;
    }

    ast::ExprStmt &parseExpressionList() {
        gc_ref_vector<ast::ExprStmt> stmts;
        Token *openCurly = consumeOptional(TokenKind::OPEN_CURLY);
        if (openCurly) {
            Token &closeCurly = parseUntilCloseCurly(stmts);
            return *new ast::ExpressionList(makeSourceSpan(openCurly->span(), closeCurly.span()), stmts);
        } else {
            ast::ExprStmt &expr = parseExpr();
            stmts.emplace_back(expr);
            return *new ast::ExpressionList(expr.sourceSpan(), stmts);
        }
    }

    ast::ExprStmt &parseCompoundStmt(Token &openCurly) {
        gc_ref_vector<ast::ExprStmt> stmts;
        Token &closeCurly = parseUntilCloseCurly(stmts);

        return *new ast::CompoundExpr(makeSourceSpan(openCurly.span(), closeCurly.span()), storageKindStack_.top(), stmts,
                                      openCurly.span().toString());
    }

    ast::ExprStmt &parseCastExpr(Token &castKeyword) {
        consumeLessThan();
        ast::ResolutionDeferredTypeRef &typeRef = parseTypeRef();
        consumeGreaterThan();
        consumeOpenParen();
        ast::ExprStmt &valueExpr = parseExpr();
        Token &closeParen = consumeCloseParen();

        return *new ast::CastExpr(
            makeSourceSpan(castKeyword.span(), closeParen.span()),
            typeRef,
            valueExpr,
            ast::CastKind::Explicit);
    }

    ast::ExprStmt &parseNewExpr(Token &newKeyword) {
        ast::ResolutionDeferredTypeRef &typeRef = parseTypeRef();
        consumeOpenParen();
        Token &closeParen = consumeCloseParen();

        return *new ast::NewExpr(
            makeSourceSpan(newKeyword.span(), closeParen.span()),
            typeRef);
    }

    ast::ExprStmt &parseConditional(Token &openingOp) {
        ast::ExprStmt &condExpr = parseExpr();
        consumeEndOfStatment();
        ast::ExprStmt &thenExpr = parseExpr();
        consumeEndOfStatment();
        ast::ExprStmt &elseExpr = parseExpr();
        Token &closingParen = consumeCloseParen();

        return *new ast::IfExprStmt(
            makeSourceSpan(openingOp.span(), closingParen.span()),
            condExpr,
            thenExpr,
            &elseExpr
        );
    }

    ast::ExprStmt &parseIfExpr(Token &ifKeyword) {
        consumeOpenParen();
        ast::ExprStmt &condExpr = parseExpr();
        consumeCloseParen();

        ast::ExprStmt &thenExpr = parseExpr();
        ast::ExprStmt *elseExpr = nullptr;
        if(consumeOptional(TokenKind::KW_ELSE)) {
            elseExpr = &parseExpr();
        }

        return *new ast::IfExprStmt(
            makeSourceSpan(ifKeyword.span(), thenExpr.sourceSpan()),
            condExpr,
            thenExpr,
            elseExpr
        );
    }

    ast::ExprStmt &parseWhile(Token &whileKeyword) {
        consumeOpenParen();
        ast::ExprStmt &condExpr = parseExpr();
        consumeCloseParen();
        ast::ExprStmt &bodyExpr = parseExpr();

        return *new ast::WhileExpr(
            makeSourceSpan(whileKeyword.span(), bodyExpr.sourceSpan()),
            condExpr,
            bodyExpr
        );
    }

    /** retval.first is parsed list of argument exprs, retval.second is the closing ')' */
    std::pair<gc_ref_vector<ast::ExprStmt>, std::reference_wrapper<Token>> parseFuncCallArguments() {
        gc_ref_vector<ast::ExprStmt> arguments;
        Token *closeParen = consumeOptional(TokenKind::CLOSE_PAREN);
        //If argument list is not empty
        if(!closeParen) {
            do {
                ast::ExprStmt &argExpr = parseExpr();
                arguments.emplace_back(argExpr);
                closeParen = &consume(TokenKind::COMMA, TokenKind::CLOSE_PAREN, ',', ')');
            } while(closeParen->kind() != TokenKind::CLOSE_PAREN);
        }
        return std::pair<gc_ref_vector<ast::ExprStmt>, std::reference_wrapper<Token>>(arguments, std::reference_wrapper<Token>(*closeParen));
    };

    ast::ExprStmt &parseFuncCallExpr(ast::ExprStmt &funcExpr, Token &openParen) {
        auto argsAndCloseParen = parseFuncCallArguments();


        return *new ast::FuncCallExpr(
            makeSourceSpan(funcExpr.sourceSpan(), argsAndCloseParen.second.get().span()),
            nullptr, //No instanceExpr
            openParen.span(),
            funcExpr,
            argsAndCloseParen.first
        );
    }

    ast::ExprStmt &parseFuncDef(Token &funcKeyword) {
        auto identifier = parseIdentifier();
        consumeColon();
        ast::ResolutionDeferredTypeRef &returnTypeRef = parseTypeRef();
        consumeOpenParen();

        gc_ref_vector<ast::ParameterDef> parameters;

        Token *closeParen = consumeOptional(TokenKind::CLOSE_PAREN);
        //If parameter list is not empty
        if(!closeParen) {
            do {
                auto name = parseIdentifier();
                consumeColon();
                ast::ResolutionDeferredTypeRef &parameterTypeRef = parseTypeRef();
                parameters.emplace_back(
                    *new ast::ParameterDef(
                        makeSourceSpan(name.span(), parameterTypeRef.sourceSpan()),
                        name,
                        parameterTypeRef
                    )
                );

                closeParen = &consume(TokenKind::COMMA, TokenKind::CLOSE_PAREN, ',', ')');
            } while(closeParen->kind() != TokenKind::CLOSE_PAREN);
        }

        storageKindStack_.push(scope::StorageKind::Local);
        ast::ExprStmt &funcBody = parseExpr();
        storageKindStack_.pop();

        return *new ast::FuncDefStmt(
            makeSourceSpan(funcKeyword.span(), funcBody.sourceSpan()),
            ast::Identifier(identifier.span(), identifier.text()),
            returnTypeRef,
            parameters,
            funcBody
        );
    }

    ast::ExprStmt &parseClassDef(Token &classKeyword) {
        auto className = parseIdentifier();

        storageKindStack_.push(scope::StorageKind::Instance);

        ast::ExprStmt &classBody = parseExpr();

        auto *compoundExpr = dynamic_cast<ast::CompoundExpr*>(&classBody);
        if(!compoundExpr) {
            gc_ref_vector<ast::ExprStmt> stmts;
            stmts.emplace_back(classBody);
            compoundExpr = new ast::CompoundExpr(classBody.sourceSpan(), scope::StorageKind::Instance, stmts, className.text());
        } else {
            compoundExpr->scope().setStorageKind(scope::StorageKind::Instance);
        }

        storageKindStack_.pop();
        if(!parsingAnonymousTemplate_) {
            return *new ast::CompleteClassDefinition(
                makeSourceSpan(classKeyword.span(), classBody.sourceSpan()),
                className,
                *compoundExpr
            );
        } else {
            return *new ast::GenericClassDefinition(
                makeSourceSpan(classKeyword.span(), classBody.sourceSpan()),
                className,
                deepCopyVector(templateParameters_),
                *compoundExpr
            );
        }
    }

    ast::ExprStmt &parseAssert(Token &assertKeyword) {
        consumeOpenParen();
        ast::ExprStmt &cond = parseExpr();
        Token &closeParen = consumeCloseParen();

        return *new ast::AssertExprStmt(
            makeSourceSpan(assertKeyword.span(), closeParen.span()),
            cond);
    }

    ast::ExprStmt &parseNamespace(Token &namespaceKeyword) {
        ast::MultiPartIdentifier namespaceId = parseQualifiedIdentifier();
        auto &body = upcast<ast::ExpressionList>(parseExpressionList());
        return *new ast::NamespaceExpr(
            makeSourceSpan(namespaceKeyword.span(), body.sourceSpan()),
            namespaceId,
            body
        );
    }

    ast::ExprStmt &parseTemplate(Token &templateKeyword) {
        if(parsingTemplate_) {
            errorStream_.error(error::ErrorKind::CannotNestTemplates, templateKeyword.span(), "Cannot nest templates");
            throw ParseAbortedException();
        }
        parsingTemplate_ = true;

        ast::Identifier *optionalId = consumeOptionalIdentifier();

        // When parsingAnonymousTemplate_ == true && !template_parameters.empty(), parseClassDef(...) will instantiate
        // GenericClassDefinitions instead of CompleteClassDefinitions
        parsingAnonymousTemplate_ = optionalId == nullptr;

        gc_ref_vector<ast::TemplateParameter> parameters = parseTemplateParameters(
            parsingAnonymousTemplate_ ? TemplateParameterRequirement::Required : TemplateParameterRequirement::Optional);

        templateParameters_ = parameters;
        auto &&body = upcast<ast::ExpressionList>(parseExpressionList());
        templateParameters_.clear();

        parsingTemplate_ = false;
        parsingAnonymousTemplate_ = false;

        if(optionalId != nullptr) {
            return *new ast::NamedTemplateExprStmt(
                makeSourceSpan(templateKeyword.span(), body.sourceSpan()),
                *optionalId,
                parameters,
                body);
        } else {
            return *new ast::AnonymousTemplateExprStmt(
                makeSourceSpan(templateKeyword.span(), body.sourceSpan()),
                parameters,
                body);
        }
    }

    enum class TemplateParameterRequirement {
        Required,
        Optional
    };

    gc_ref_vector <ast::TemplateParameter> parseTemplateParameters(TemplateParameterRequirement requirement) {
        gc_ref_vector<ast::TemplateParameter> parameters;

        if(requirement == TemplateParameterRequirement::Required) {
            consume(TokenKind::OP_LT, '<');
        } else {
            if(!consumeOptional(TokenKind::OP_LT)) {
                return parameters;
            }
        }
        if(!consumeOptional(TokenKind::OP_GT)) {
            do {
                auto identifier = parseIdentifier();
                parameters.emplace_back(*new ast::TemplateParameter(identifier.span(), identifier));
            } while(consume(TokenKind::COMMA, TokenKind::OP_GT, "',' or '>'").kind() == TokenKind::COMMA);
        }
        return parameters;
    }

    ast::ExprStmt &parseExpand(Token &expandKeyword) {
        gc_ref_vector<ast::TypeRef> templateArgs;
        ast::MultiPartIdentifier templateName = parseQualifiedIdentifier();
        SourceSpan endSpan = templateName.span();

        if(consumeOptional(TokenKind::OP_LT)) {
            if(!consumeOptional(TokenKind::OP_GT)) {
                Token *terminator = nullptr;
                do {
                    templateArgs.emplace_back(parseTypeRef());
                } while ((terminator = &consume(TokenKind::OP_GT, TokenKind::COMMA, "'>' or ','"))->kind() == TokenKind::COMMA);
                endSpan = terminator->span();
            }
        }

        return *new ast::TemplateExpansionExprStmt(
            makeSourceSpan(expandKeyword.span(), endSpan),
            templateName, templateArgs);
    }

    ast::ExprStmt &parseDotExpr(ast::ExprStmt &lValue, Token &operatorToken) {
        auto memberName = parseIdentifier();

        if(Token *openParen = consumeOptional(TokenKind::OPEN_PAREN)) {
            auto argsAndCloseParen = parseFuncCallArguments();

            return *new ast::FuncCallExpr(
                makeSourceSpan(lValue.sourceSpan(), argsAndCloseParen.second.get().span()),
                &lValue,
                openParen->span(),
                *new ast::MethodRefExpr(memberName),
                argsAndCloseParen.first
            );
        }

        return *new ast::DotExpr(
            makeSourceSpan(lValue.sourceSpan(), memberName.span()),
            operatorToken.span(),
            lValue,
            memberName
        );
    }

    ast::ExprStmt &parseBinaryExpr(ast::ExprStmt &lValue, Token &operatorToken);

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
        registerGenericParselet(TokenKind::KW_NAMESPACE, std::bind(&AnodeParser::parseNamespace, this, _1));
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

    ast::Module &parseModule() {
        storageKindStack_.push(scope::StorageKind::Global);
        gc_ref_vector<ast::ExprStmt> exprs;

        while(!lexer_.eof()) {
            exprs.emplace_back(parseExpr());
        }

        ast::CompoundExpr &body = *new ast::CompoundExpr(source::SourceSpan::Any, scope::StorageKind::Global, exprs, lexer_.inputName());

        ASSERT(storageKindStack_.size() == 1);
        storageKindStack_.pop();

        return *new ast::Module(lexer_.inputName(), body);
    }
};

}}}