#include "ast.h"
#include "error.h"

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

            virtual void enterVariableRefExpr(LwnnParser::VariableRefExprContext * ctx) override {
                setResult(std::make_unique<VariableRefExpr>(getSourceSpan(ctx), ctx->getText()));
            }
        };

        class StatementListener : public LwnnBaseListenerHelper<ast::Stmt> {
        public:

            virtual void enterStatement(LwnnParser::StatementContext *ctx) override {
                LwnnParser::ExprContext *exprCtx = ctx->expr();
                ExprListener listener;
                ctx->expr()->enterRule(&listener);
                if(!listener.hasResult()) return;
                setResult(listener.surrenderResult());
            }
        };

        class CompiledUnitListener : public LwnnBaseListenerHelper<ast::Module> {
            std::string moduleName_;

        public:
            CompiledUnitListener(const std::string &moduleName_) : moduleName_(moduleName_) {}

            virtual void enterModule(LwnnParser::ModuleContext *ctx) override {
                std::vector<LwnnParser::StatementContext*> statements = ctx->statement();
                auto module = std::make_unique<ast::Module>(getSourceSpan(ctx), moduleName_);;
                for (LwnnParser::StatementContext *stmt: statements) {
                    StatementListener listener;
                    stmt->enterRule(&listener);
                    if(!listener.hasResult()) return;
                    module->addStatement(listener.surrenderResult());
                }
                setResult(std::move(module));
            }
        };

        class LwnnErrorListener : public DiagnosticErrorListener {
            error::ErrorStream &errorStream_;
        public:
            LwnnErrorListener(error::ErrorStream &errorStream_)
                : DiagnosticErrorListener(true),
                  errorStream_(errorStream_) {}

            virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line,
                                     size_t charPositionInLine, const std::string &msg, std::exception_ptr e) {

                source::SourceSpan span = getSourceSpan(offendingSymbol);
                errorStream_.error(span, msg);
            };
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

            CompiledUnitListener listener{inputName};

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
