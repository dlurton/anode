#include "ast.h"
#include "error.h"

#include <antlr4-runtime.h>

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
        static source::SourceSpan getSourceSpan(antlr4::ParserRuleContext *startContext,
                                        antlr4::ParserRuleContext *endContext) {
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
                setResult(listener.surrenderResult());
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

                ExprListener rightListener;
                ctx->right->enterRule(&rightListener);

                BinaryOperationKind opKind;

                switch(ctx->op->getType()) {
                    case LwnnParser::OP_ADD: opKind = BinaryOperationKind::Add; break;
                    case LwnnParser::OP_SUB: opKind = BinaryOperationKind::Sub; break;
                    case LwnnParser::OP_MUL: opKind = BinaryOperationKind::Mul; break;
                    case LwnnParser::OP_DIV: opKind = BinaryOperationKind::Div; break;
                    default:
                        ASSERT_FAIL("Unhandled Token Type (Operators)");
                }

                setResult(std::make_unique<BinaryExpr>(getSourceSpan(ctx->left, ctx->right),
                    leftListener.surrenderResult(), opKind, rightListener.surrenderResult()));
            }

            virtual void enterVariableRefExpr(LwnnParser::VariableRefExprContext * ctx) override {
                setResult(std::make_unique<VariableRefExpr>(getSourceSpan(ctx), ctx->getText()));
            }
        };

        class VarDeclListener : public LwnnBaseListenerHelper<ast::VariableDeclExpr> {
        public:
            virtual void enterVarDecl(LwnnParser::VarDeclContext *ctx) override {
                auto typeRef = std::make_unique<TypeRef>(
                    getSourceSpan(ctx->type),
                    ctx->type->getText());

                std::unique_ptr<ast::ExprStmt> initializer;

                if(ctx->initializer) {
                    ExprListener exprListener;
                    ctx->initializer->enterRule(&exprListener);
                    initializer = exprListener.surrenderResult();
                }

                setResult(std::make_unique<ast::VariableDeclExpr>(
                    getSourceSpan(ctx),
                    ctx->name->getText(),
                    std::move(typeRef),
                    std::move(initializer)
                ));
            }
        };

        class StatementListener : public LwnnBaseListenerHelper<ast::Stmt> {
        public:

            virtual void enterStatement(LwnnParser::StatementContext *ctx) override {
                LwnnParser::VarDeclContext *varDeclCtx = ctx->varDecl();
                LwnnParser::ExprContext *exprCtx = ctx->expr();

                if(varDeclCtx != nullptr) {
                    VarDeclListener varDeclListener;
                    varDeclCtx->enterRule(&varDeclListener);
                    setResult(varDeclListener.surrenderResult());
                } else if(exprCtx) {
                    ExprListener listener;
                    ctx->expr()->enterRule(&listener);
                    setResult(listener.surrenderResult());
                }
            }
        };

        class CompiledUnitListener : public LwnnBaseListenerHelper<ast::Module> {
        public:
            virtual void enterModule(LwnnParser::ModuleContext *ctx) override {
                std::vector<LwnnParser::StatementContext*> statements = ctx->statement();
                std::unique_ptr<Stmt> body;

                //Don't wrap a single global statement of a module in a BlockStmt because
                //that hides it's return type which is needed for the REPL.
                if(statements.size() == 1) {
                    StatementListener listener;
                    statements[0]->enterRule(&listener);
                    body = listener.surrenderResult();
                    ASSERT(body);
                }
                else {
                    auto block = std::make_unique<ast::CompoundStmt>(getSourceSpan(ctx));
                    for (LwnnParser::StatementContext *stmt: statements) {
                        StatementListener listener;
                        stmt->enterRule(&listener);
                        block->addStatement(listener.surrenderResult());
                    }
                    body = std::move(block);
                }
                auto module = std::make_unique<ast::Module>("temp_module_name", std::move(body));
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
            lexer.removeErrorListeners();
            lexer.addErrorListener(&errorListener);
            inputStream.name = inputName;

            CommonTokenStream tokens{&lexer};
            tokens.fill();

            LwnnParser parser(&tokens);
            parser.removeErrorListeners();
            parser.addErrorListener(&errorListener);

            auto *moduleCtx = parser.module();

            CompiledUnitListener listener;
            moduleCtx->enterRule(&listener);

            if(errorStream.errorCount() > 0) {
                return nullptr;
            }

            return listener.surrenderResult();
        }
    }
}
