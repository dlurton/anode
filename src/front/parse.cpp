#include "front/ast.h"
#include "front/error.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <antlr4-runtime.h>
#pragma GCC diagnostic pop

#include "generated/LwnnLexer.h"
#include "generated/LwnnParser.h"
#include "generated/LwnnBaseListener.h"

using namespace lwnn::ast;
using namespace lwnn_parser;
using namespace antlr4;

namespace lwnn {
    /** This namespace contains types and functions mainly associated with invoking the generated
     * ANTLR4 parser and converting parse tree to the LWNN AST.
     */
    namespace parse {

        std::unique_ptr<ast::ExprStmt> extractExpr(LwnnParser::ExprContext *ctx);
        std::unique_ptr<ast::ExprStmt> extractStatement(LwnnParser::StatementContext *ctx);
        std::unique_ptr<ast::CompoundExprStmt> extractCompoundExprStmt(LwnnParser::CompoundExprStmtContext *ctx);

        /** Extracts a SourceRange from values specified in token. */
        static source::SourceSpan getSourceSpan(antlr4::Token *token) {
            auto startSource = token->getTokenSource();

            //NOTE:  this is kinda not so good because it assumes tokens never span lines, which is fine for most
            //tokens since they don't span lines.  But if I ever decide to implement multi-line strings...
            return source::SourceSpan(startSource->getSourceName(),
                  source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
                  source::SourceLocation(startSource->getLine(),
                  startSource->getCharPositionInLine() + token->getText().length()));
        }

        /** Extracts a SourceRange from values specified in ctx. */
        static source::SourceSpan getSourceSpan(antlr4::ParserRuleContext *ctx) {
            auto startSource = ctx->getStart()->getTokenSource();
            auto endSource = ctx->getStop()->getTokenSource();

            return source::SourceSpan(startSource->getSourceName(),
                      source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
                      source::SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
        }

        /** Extracts a SourceRange from values specified in ctx. */
        static source::SourceSpan getSourceSpan(antlr4::ParserRuleContext *startContext, antlr4::ParserRuleContext *endContext) {
            auto startSource = startContext->getStart()->getTokenSource();
            auto endSource = endContext->getStop()->getTokenSource();

            return source::SourceSpan(startSource->getSourceName(),
                      source::SourceLocation(startSource->getLine(), startSource->getCharPositionInLine()),
                      source::SourceLocation(endSource->getLine(), endSource->getCharPositionInLine()));
        }

        /*******************************************/
        template<typename TAstNode>
        class LwnnBaseListenerHelper : public LwnnBaseListener {
            std::unique_ptr<TAstNode> resultNode_;
        protected:
            void setResult(std::unique_ptr<TAstNode> node) {
                resultNode_ = std::move(node);
            }
        public:
            bool hasResult() { return resultNode_ != nullptr; }

            std::unique_ptr<TAstNode> surrenderResult() {
                ASSERT(resultNode_ != nullptr && "Result has not been set");
                return std::move(resultNode_);
            }
        };

        /*******************************************/
        class ExprListener : public LwnnBaseListenerHelper<ast::ExprStmt> {
        public:
            virtual void enterParensExpr(LwnnParser::ParensExprContext *ctx) override {
                ExprListener listener;
                ctx->expr()->enterRule(&listener);
                if(!listener.hasResult()) return;
                setResult(listener.surrenderResult());
            }

            virtual void enterVarDeclExpr(LwnnParser::VarDeclExprContext *ctx) override {
                auto typeRef = std::make_unique<TypeRef>(
                    getSourceSpan(ctx->type),
                    ctx->type->getText());

                setResult(std::make_unique<ast::VariableDeclExpr>(
                    getSourceSpan(ctx),
                    ctx->name->getText(),
                    std::move(typeRef)
                ));
            }

            virtual void enterCastExpr(LwnnParser::CastExprContext *ctx) override {
                ExprListener listener;
                ctx->expr()->enterRule(&listener);
                if(!listener.hasResult()) return;

                source::SourceSpan sourceSpan = getSourceSpan(ctx);
                auto typeRef = std::make_unique<TypeRef>(getSourceSpan(ctx), ctx->type->getText());

                auto castExpr = std::make_unique<CastExpr>(
                    getSourceSpan(ctx),
                    std::move(typeRef),
                    listener.surrenderResult(),
                    CastKind::Explicit);

                setResult(std::move(castExpr));
            }

            virtual void enterLiteralBool(LwnnParser::LiteralBoolContext *ctx) override {
                if(ctx->getText() == "true") {
                    setResult(std::make_unique<LiteralBoolExpr>(getSourceSpan(ctx), true));
                } else if(ctx->getText() == "false") {
                    setResult(std::make_unique<LiteralBoolExpr>(getSourceSpan(ctx), false));
                } else {
                    ASSERT_FAIL("Parser parsed something that it thinks is a boolean literal but wasn't 'true' or 'false'.");
                }
            }

            virtual void enterLiteralInt32Expr(LwnnParser::LiteralInt32ExprContext *ctx) override {
                int value = std::stoi(ctx->getText());
                setResult(std::make_unique<LiteralInt32Expr>(getSourceSpan(ctx), value));
            }

            virtual void enterLiteralFloatExpr(LwnnParser::LiteralFloatExprContext *ctx) override {
                double value = std::stof(ctx->getText());
                setResult(std::make_unique<LiteralFloatExpr>(getSourceSpan(ctx), value));
            }

            virtual void enterBinaryExpr(LwnnParser::BinaryExprContext *ctx) override {
                ExprListener leftListener;
                ctx->left->enterRule(&leftListener);
                if(!leftListener.hasResult()) return;

                ExprListener rightListener;
                ctx->right->enterRule(&rightListener);
                if(!rightListener.hasResult()) return;

                BinaryOperationKind opKind;
                std::unique_ptr<ExprStmt> leftExpr = leftListener.surrenderResult();

                switch(ctx->op->getType()) {
                    case LwnnParser::OP_ASSIGN: {
                        opKind = BinaryOperationKind::Assign;
                        ast::VariableRefExpr *varRef = dynamic_cast<ast::VariableRefExpr*>(leftExpr.get());
                        if(varRef != nullptr) {
                            varRef->setVariableAccess(VariableAccess::Write);
                        }
                        break;
                    }
                    case LwnnParser::OP_ADD:    opKind = BinaryOperationKind::Add; break;
                    case LwnnParser::OP_SUB:    opKind = BinaryOperationKind::Sub; break;
                    case LwnnParser::OP_MUL:    opKind = BinaryOperationKind::Mul; break;
                    case LwnnParser::OP_DIV:    opKind = BinaryOperationKind::Div; break;
                    case LwnnParser::OP_EQ:     opKind = BinaryOperationKind::Eq; break;
                    default:
                        ASSERT_FAIL("Unhandled Token Type (Operators)");
                }


                std::unique_ptr<ExprStmt> rightExpr = rightListener.surrenderResult();
                auto node = std::make_unique<BinaryExpr>(
                                    getSourceSpan(ctx->left, ctx->right),
                                    std::move(leftExpr),
                                    opKind,
                                    getSourceSpan(ctx->op),
                                    std::move(rightExpr));
                setResult(std::move(node));
            }

            virtual void enterVariableRefExpr(LwnnParser::VariableRefExprContext *ctx) override {
                setResult(std::make_unique<VariableRefExpr>(getSourceSpan(ctx), ctx->getText()));
            }

            virtual void enterTernaryExpr(LwnnParser::TernaryExprContext *ctx) override {
                if(!ctx->cond) return;
                ExprListener condListener;
                ctx->cond->enterRule(&condListener);
                if(!condListener.hasResult()) return;

                if(!ctx->thenExpr) return;
                ExprListener thenListener;
                ctx->thenExpr->enterRule(&thenListener);
                if(!thenListener.hasResult()) return;

                std::unique_ptr<ExprStmt> elseExpr;

                if(ctx->elseExpr) {
                    ExprListener elseListener;
                    ctx->elseExpr->enterRule(&elseListener);
                    if (!thenListener.hasResult()) return;
                    elseExpr = elseListener.surrenderResult();
                }

                setResult(
                    std::make_unique<IfExprStmt>(getSourceSpan(ctx),
                                             condListener.surrenderResult(),
                                             thenListener.surrenderResult(),
                                             std::move(elseExpr)));
            }

            virtual void enterIfExpr(LwnnParser::IfExprContext *ctx) override {
                ASSERT(ctx->cond);
                ExprListener condListener;
                ctx->cond->enterRule(&condListener);
                if(!condListener.hasResult()) return;

                std::unique_ptr<ast::ExprStmt> thenExprStmt = extractExpr(ctx->thenExpr);
                std::unique_ptr<ast::ExprStmt> elseExprStmt = extractExpr(ctx->elseExpr);

                setResult(
                    std::make_unique<IfExprStmt>(getSourceSpan(ctx),
                                                 condListener.surrenderResult(),
                                                 std::move(thenExprStmt),
                                                 std::move(elseExprStmt)));
            }

            virtual void enterCompoundExpr(LwnnParser::CompoundExprContext * ctx) override {
                setResult(extractCompoundExprStmt(ctx->compoundExprStmt()));
            }
        };

        std::unique_ptr<ast::ExprStmt> extractExpr(LwnnParser::ExprContext *ctx) {
            if(!ctx) return nullptr;
            ExprListener listener;
            ctx->enterRule(&listener);
            return listener.hasResult() ? listener.surrenderResult() : nullptr;
        }

        /*******************************************/
        class StatementListener : public LwnnBaseListenerHelper<ast::ExprStmt> {
        public:

            virtual void enterExprStmt(LwnnParser::ExprStmtContext *ctx) override {
                ExprListener listener;
                ctx->expr()->enterRule(&listener);
                if(!listener.hasResult()) return;
                setResult(listener.surrenderResult());
            }
            virtual void enterIfStmt(LwnnParser::IfStmtContext *ctx) override {
                ASSERT(ctx->cond);
                ExprListener condListener;
                ctx->cond->enterRule(&condListener);
                if(!condListener.hasResult()) return;

                std::unique_ptr<ast::ExprStmt> thenExprStmt = extractStatement(ctx->thenStmt);
                std::unique_ptr<ast::ExprStmt> elseExprStmt = extractStatement(ctx->elseStmt);

                setResult(
                    std::make_unique<IfExprStmt>(getSourceSpan(ctx),
                                                 condListener.surrenderResult(),
                                                 std::move(thenExprStmt),
                                                 std::move(elseExprStmt)));
            }

            virtual void enterCompoundStmt(LwnnParser::CompoundStmtContext *ctx) override {
                setResult(extractCompoundExprStmt(ctx->compoundExprStmt()));
            }
        };

        std::unique_ptr<ast::ExprStmt> extractStatement(LwnnParser::StatementContext *ctx) {
            if(!ctx) return nullptr;

            StatementListener listener;
            ctx->enterRule(&listener);
            return listener.hasResult() ? listener.surrenderResult() : nullptr;
        }

        /*******************************************/
        class CompoundExprStmtListener : public LwnnBaseListenerHelper<ast::CompoundExprStmt> {
        public:
            virtual void enterCompoundExprStmt(LwnnParser::CompoundExprStmtContext * ctx) override {
                std::vector<LwnnParser::StatementContext*> expressions = ctx->statement();
                auto compoundExpr = std::make_unique<ast::CompoundExprStmt>(getSourceSpan(ctx));
                for (LwnnParser::StatementContext *expr : expressions) {
                    std::unique_ptr<ExprStmt> exprStmt = extractStatement(expr);
                    if(exprStmt) {
                        compoundExpr->addStatement(std::move(exprStmt));
                    }
                }
                setResult(std::move(compoundExpr));
            }
        };

        std::unique_ptr<ast::CompoundExprStmt> extractCompoundExprStmt(LwnnParser::CompoundExprStmtContext *ctx) {
            if(!ctx) return nullptr;
            CompoundExprStmtListener listener;
            ctx->enterRule(&listener);
            return listener.hasResult() ? listener.surrenderResult() : nullptr;
        }
        /*******************************************/
        class ModuleListener : public LwnnBaseListenerHelper<ast::Module> {
            std::string moduleName_;

        public:
            ModuleListener(const std::string &moduleName_) : moduleName_(moduleName_) {}

            virtual void enterModule(LwnnParser::ModuleContext *ctx) override {
                std::vector<LwnnParser::StatementContext*> statements = ctx->statement();
                auto compoundExpr = std::make_unique<ast::CompoundExprStmt>(getSourceSpan(ctx));
                auto module = std::make_unique<ast::Module>( moduleName_, std::move(compoundExpr));;
                for (LwnnParser::StatementContext *stmt: statements) {
                    StatementListener listener;
                    stmt->enterRule(&listener);
                    if(!listener.hasResult()) return;
                    module->body()->addStatement(listener.surrenderResult());
                }
                setResult(std::move(module));
            }
        };

        /*******************************************/
        class LwnnErrorListener : public BaseErrorListener {
            error::ErrorStream &errorStream_;
            std::string inputName_;
        public:
            LwnnErrorListener(error::ErrorStream &errorStream_)
                : errorStream_(errorStream_) {}

            virtual void syntaxError(Recognizer *, Token *offendingSymbol, size_t line,
                                     size_t charPositionInLine, const std::string &msg, std::exception_ptr) {

                //If offendingSymbol is null, error was from lexer and we don't even have an error token yet.
                if(offendingSymbol == nullptr) {
                    //We can create a new source span--this one will only have a length of 1, though...
                    source::SourceSpan span {
                        inputName_,
                        source::SourceLocation { line, charPositionInLine },
                        source::SourceLocation { line, charPositionInLine + 1 }
                    };
                    errorStream_.error(span, msg);
                } else {
                    //However if we do have the offendingSymbol we should use that because the span generated
                    //from it should have the span's full length.
                    errorStream_.error(getSourceSpan(offendingSymbol), msg);
                }
            };

            virtual void reportAmbiguity(Parser *, const dfa::DFA &, size_t , size_t , bool ,
                                         const antlrcpp::BitSet &, atn::ATNConfigSet *) override {
                // May want to grab code from here to determine how to report this intelligently
                // https://github.com/antlr/antlr4/blob/master/runtime/Cpp/runtime/src/DiagnosticErrorListener.cpp
//                source::SourceSpan span {
//                    inputName_,
//                    source::SourceLocation { 1, 1},
//                    source::SourceLocation { 1, 1 }
//                };
//                errorStream_.warning(span, "Grammar is ambiguous.  I think this is an bug in Lwnn.g4.");
            }

            virtual void reportAttemptingFullContext(Parser *, const dfa::DFA &, size_t , size_t ,
                                                     const antlrcpp::BitSet &, atn::ATNConfigSet *) override  {
            }

            virtual void reportContextSensitivity(Parser *, const dfa::DFA &, size_t , size_t ,
                                                  size_t , atn::ATNConfigSet *) override  {
            }
        };

        std::unique_ptr<Module> parseModule(const std::string &lineOfCode, const std::string &inputName) {
            error::ErrorStream errorStream{std::cerr};
            LwnnErrorListener errorListener{errorStream};

            ANTLRInputStream inputStream{lineOfCode};
            LwnnLexer lexer{&inputStream};
            inputStream.name = inputName;

            lexer.removeErrorListeners();
            lexer.addErrorListener(&errorListener);

            CommonTokenStream tokens{&lexer};
            tokens.fill();

            if(errorStream.errorCount() > 0) return nullptr;

            LwnnParser parser(&tokens);
            parser.removeErrorListeners();
            parser.addErrorListener(&errorListener);

            ModuleListener listener{inputName};

            auto *moduleCtx = parser.module();

            moduleCtx->enterRule(&listener);
            if(errorStream.errorCount() > 0) return nullptr;
            if(!listener.hasResult()) {
                std::cerr << "Nothing was parsed!\n";
                return nullptr;
            }

            return listener.surrenderResult();
        }
    }
}
