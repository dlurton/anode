#include "emit.h"
#include "CompileAstVisitor.h"

namespace lwnn {
    namespace back {

        class ExprAstVisitor : public CompileAstVisitor {
            std::stack<llvm::Value *> valueStack_;

        public:
            ExprAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}

            bool hasValue() {
                return valueStack_.size() > 0;
            }

            llvm::Value *llvmValue() {
                ASSERT(valueStack_.size() == 1);
                return valueStack_.top();
            }

            virtual void visitedCastExpr(ast::CastExpr *expr) {
                llvm::Value *value = valueStack_.top();
                valueStack_.pop();

                ASSERT(expr->type()->isPrimitive());
                ASSERT(expr->valueExpr()->type()->isPrimitive());

                llvm::Value *castedValue = nullptr;
                llvm::Type *destLlvmType = cc().toLlvmType(expr->type());
                switch (expr->valueExpr()->type()->primitiveType()) {
                    //From int32
                    case type::PrimitiveType::Int32:
                        switch (expr->type()->primitiveType()) {
                            //To bool
                            case type::PrimitiveType::Bool: {
                                castedValue = cc().irBuilder().CreateICmpNE(value, cc().getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            }
                                //To int
                            case type::PrimitiveType::Int32:
                                castedValue = value;
                                break;
                                //to float
                            case type::PrimitiveType::Float:
                                castedValue = cc().irBuilder().CreateSIToFP(value, destLlvmType);
                                break;
                            default:
                                ASSERT_FAIL("Unknown cast from int32");
                        }
                        break;
                        //From float
                    case type::PrimitiveType::Float:
                        switch (expr->type()->primitiveType()) {
                            //To to int32 or bool
                            case type::PrimitiveType::Bool:
                                castedValue = cc().irBuilder().CreateFCmpUNE(value, cc().getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            case type::PrimitiveType::Int32:
                                castedValue = cc().irBuilder().CreateFPToSI(value, destLlvmType);
                                break;
                            case type::PrimitiveType::Float:
                                castedValue = value;
                                break;
                            default:
                                ASSERT_FAIL("Unknown cast from float");
                        }
                        break;
                        //From bool
                    case type::PrimitiveType::Bool:
                        ASSERT_FAIL("cannot cast from bool to anything");
//                        switch(expr->type()->primitiveType()) {
//                            //To bool
//                            case type::PrimitiveType::Bool:
//                                castedValue = value;
//                                break;
//                            //To to int32
//                            case type::PrimitiveType::Int32:
//                                castedValue = irBuilder_.CreateSExt(value, destLlvmType);
//                                break;
//                                //to float
//                            case type::PrimitiveType::Float:
//                                castedValue = irBuilder_.CreateSIToFP(value, destLlvmType);
//                                break;
//                            default:
//                                ASSERT_FAIL("Unknown cast from bool");
//                        }
                        break;
                    default:
                        ASSERT_FAIL("Unhandled type::PrimitiveType")
                }
                ASSERT(castedValue && "Cannot fail to create casted value...");
                valueStack_.push(castedValue);
            }

            virtual void visitedVariableDeclExpr(ast::VariableDeclExpr *expr) override {
                //For now assume global variables.
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                llvm::GlobalVariable *globalVariable = cc().llvmModule().getNamedGlobal(expr->name());
                ASSERT(globalVariable);
                globalVariable->setAlignment(ALIGNMENT);
                globalVariable->setInitializer(cc().getDefaultValueForType(expr->type()));

                visitVariableRefExpr(expr);
            }

            virtual void visitLiteralInt32Expr(ast::LiteralInt32Expr *expr) override {
                valueStack_.push(llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t) expr->value(), true)));
            }

            virtual void visitLiteralFloatExpr(ast::LiteralFloatExpr *expr) override {
                valueStack_.push(llvm::ConstantFP::get(cc().llvmContext(), llvm::APFloat(expr->value())));
            }

            virtual void visitLiteralBoolExpr(ast::LiteralBoolExpr *expr) {
                valueStack_.push(llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, (uint64_t) expr->value(), true)));
            }

            virtual void visitVariableRefExpr(ast::VariableRefExpr *expr) override {
                //We are assuming global variables for now
                //llvm::Value *allocaInst = lookupVariable(expr->name());

                llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(expr->name());
                ASSERT(globalVar);

                if (expr->variableAccess() == ast::VariableAccess::Write) {
                    valueStack_.push(globalVar);
                    return;
                }

                llvm::LoadInst *loadInst = cc().irBuilder().CreateLoad(globalVar);
                loadInst->setAlignment(ALIGNMENT);
                valueStack_.push(loadInst);

            }

            virtual void visitedBinaryExpr(ast::BinaryExpr *expr) override {
                ASSERT(expr->lValue()->type() == expr->rValue()->type() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                ASSERT(expr->type()->isPrimitive() && "Only primitive types currently supported here");

                if (expr->operation() == ast::BinaryOperationKind::Assign) {
                    cc().irBuilder().CreateStore(rValue, lValue); //(args swapped on purpose)
                    //Note:  I think this will call rValue a second time if rValue is a call site.
                    valueStack_.push(rValue);
                    return;
                }
                llvm::Value *resultValue = nullptr;
                switch (expr->operandsType()->primitiveType()) {
                    case type::PrimitiveType::Bool:
                        switch (expr->operation()) {
                            case ast::BinaryOperationKind::Eq:
                                resultValue = cc().irBuilder().CreateICmpEQ(lValue, rValue);
                                break;
//                            case BinaryOperationKind::And:
//                                resultValue =  irBuilder_.CreateICmpEQ(lValue, rValue);
//                                break;
//                            case BinaryOperationKind::Or:
//                                resultValue =  irBuilder_.CreateICmpEQ(lValue, rValue);
//                                break;
                            default:
                                ASSERT_FAIL("Unsupported BinaryOperationKind for bool primitive type")
                        }
                        break;
                    case type::PrimitiveType::Int32:
                        switch (expr->operation()) {
                            case ast::BinaryOperationKind::Eq:
                                resultValue = cc().irBuilder().CreateICmpEQ(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Add:
                                resultValue = cc().irBuilder().CreateAdd(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Sub:
                                resultValue = cc().irBuilder().CreateSub(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Mul:
                                resultValue = cc().irBuilder().CreateMul(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Div:
                                resultValue = cc().irBuilder().CreateSDiv(lValue, rValue);
                                break;
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                        break;
                    case type::PrimitiveType::Float:
                        switch (expr->operation()) {
                            case ast::BinaryOperationKind::Eq:
                                resultValue = cc().irBuilder().CreateFCmpOEQ(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Add:
                                resultValue = cc().irBuilder().CreateFAdd(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Sub:
                                resultValue = cc().irBuilder().CreateFSub(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Mul:
                                resultValue = cc().irBuilder().CreateFMul(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::Div:
                                resultValue = cc().irBuilder().CreateFDiv(lValue, rValue);
                                break;
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                        break;
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
                ASSERT(resultValue);
                valueStack_.push(resultValue);
            }

            virtual bool visitingIfExpr(ast::IfExprStmt *ifExpr) override {
                //This function is modeled after: https://llvm.org/docs/tutorial/LangImpl08.html (ctrl-f for "IfExprAST::codegen")
                //Emit the condition
                llvm::Value *condValue = emitExpr(ifExpr->condition(), cc());

                //Prepare the BasicBlocks
                llvm::Function *currentFunc = cc().irBuilder().GetInsertBlock()->getParent();
                llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(cc().llvmContext(), "thenBlock");
                llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(cc().llvmContext(), "elseBlock");
                llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "endBlock");

                //Branch to then or else blocks, depending on condition.
                cc().irBuilder().CreateCondBr(condValue, thenBlock, elseBlock);

                //Emit the thenBlock
                currentFunc->getBasicBlockList().push_back(thenBlock);
                cc().irBuilder().SetInsertPoint(thenBlock);
                llvm::Value *thenValue = emitExpr(ifExpr->thenExpr(), cc());
                thenBlock = cc().irBuilder().GetInsertBlock();

                //Jump to the endBlock, skipping the elseBlock
                cc().irBuilder().CreateBr(endBlock);

                //Emit the elseBlock
                currentFunc->getBasicBlockList().push_back(elseBlock);
                cc().irBuilder().SetInsertPoint(elseBlock);

                llvm::Value *elseValue = nullptr;
                if (ifExpr->elseExpr()) {
                    elseValue = emitExpr(ifExpr->elseExpr(), cc());
                }
                cc().irBuilder().CreateBr(endBlock);
                elseBlock = cc().irBuilder().GetInsertBlock();


                //Emit the endBlock
                currentFunc->getBasicBlockList().push_back(endBlock);
                cc().irBuilder().SetInsertPoint(endBlock);
                if (ifExpr->type()->primitiveType() != type::PrimitiveType::Void) {
                    llvm::PHINode *phi = cc().irBuilder().CreatePHI(cc().toLlvmType(ifExpr->type()),
                                                                    ifExpr->elseExpr() ? 2 : 1, "iftmp");
                    phi->addIncoming(thenValue, thenBlock);
                    if (ifExpr->elseExpr()) {
                        phi->addIncoming(elseValue, elseBlock);
                    }
                    valueStack_.push(phi);
                }
                return false;
            }

            virtual bool visitingCompoundExpr(ast::CompoundExprStmt *expr) override {

                llvm::Value *lastValue = nullptr;
                for (ast::ExprStmt *expr : expr->statements()) {
                    ExprAstVisitor exprAstVisitor{cc()};
                    expr->accept(&exprAstVisitor);
                    ASSERT(exprAstVisitor.hasValue());
                    lastValue = exprAstVisitor.llvmValue();
                }
                valueStack_.push(lastValue);

                return false;
            }

        private:

        };

        llvm::Value *emitExpr(ast::ExprStmt *exprStmt, CompileContext &cc) {
            ExprAstVisitor visitor{cc};
            exprStmt->accept(&visitor);

            return visitor.hasValue() ? visitor.llvmValue() : nullptr;
        }
    }
}