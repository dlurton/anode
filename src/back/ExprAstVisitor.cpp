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
                            case type::PrimitiveType::Bool:
                                castedValue = cc().irBuilder().CreateICmpNE(value, cc().getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            //To int
                            case type::PrimitiveType::Int32:
                                castedValue = value;
                                break;
                            //To float
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

            virtual bool visitingBinaryExpr(ast::BinaryExpr *expr) override {
                if(expr->binaryExprKind() != ast::BinaryExprKind::Logical) {
                    return true;
                }

                //Prepare the basic blocks
                llvm::BasicBlock *lValueBlock = cc().irBuilder().GetInsertBlock();
                llvm::Function *currentFunc = lValueBlock->getParent();
                llvm::BasicBlock *rValueBlock = llvm::BasicBlock::Create(cc().llvmContext(), "rValueBlock");
                llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "endBlock");

                //Emit the lValue, which is always the first operand of a logical operation to be evaluated
                llvm::Value *lValue = emitExpr(expr->lValue(), cc());

                bool lValueConst;
                switch(expr->operation()) {
                    //For the && operator, only evaluate the lValue if the rValue is true
                    case ast::BinaryOperationKind::LogicalAnd:
                        cc().irBuilder().CreateCondBr(lValue, rValueBlock, endBlock);
                        lValueConst = false;
                        break;
                    case ast::BinaryOperationKind::LogicalOr:
                        cc().irBuilder().CreateCondBr(lValue, endBlock, rValueBlock);
                        lValueConst = true;
                        break;
                    default:
                        ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be a logical operator)")
                }
                currentFunc->getBasicBlockList().push_back(rValueBlock);
                cc().irBuilder().SetInsertPoint(rValueBlock);
                llvm::Value *rValue = emitExpr(expr->rValue(), cc());
                cc().irBuilder().CreateBr(endBlock);

                currentFunc->getBasicBlockList().push_back(endBlock);
                cc().irBuilder().SetInsertPoint(endBlock);
                llvm::PHINode *phi = cc().irBuilder().CreatePHI(cc().toLlvmType(type::PrimitiveType::Bool), 2, "logical_tmp");

                llvm::ConstantInt *shortCircuitLlvmValue
                    = llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, (uint64_t) lValueConst, false));
                phi->addIncoming(shortCircuitLlvmValue, lValueBlock);
                phi->addIncoming(rValue, rValueBlock);

                valueStack_.push(phi);
                return false;
            }

            virtual void visitedBinaryExpr(ast::BinaryExpr *expr) override {
                if(expr->binaryExprKind() != ast::BinaryExprKind::Arithmetic) {
                    return;
                }

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
                            case ast::BinaryOperationKind::NotEq:
                                resultValue = cc().irBuilder().CreateICmpNE(lValue, rValue);
                                break;
                            default:
                                ASSERT_FAIL("Unsupported BinaryOperationKind for bool primitive type")
                        }
                        break;
                    case type::PrimitiveType::Int32:
                        switch (expr->operation()) {
                            case ast::BinaryOperationKind::Eq:
                                resultValue = cc().irBuilder().CreateICmpEQ(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::NotEq:
                                resultValue = cc().irBuilder().CreateICmpNE(lValue, rValue);
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
                            case ast::BinaryOperationKind::GreaterThan:
                                resultValue = cc().irBuilder().CreateICmpSGT(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::GreaterThanOrEqual:
                                resultValue = cc().irBuilder().CreateICmpSGE(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::LessThan:
                                resultValue = cc().irBuilder().CreateICmpSLT(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::LessThanOrEqual:
                                resultValue = cc().irBuilder().CreateICmpSLE(lValue, rValue);
                                break;
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be an arithmetic operator)");
                        }
                        break;
                    case type::PrimitiveType::Float:
                        switch (expr->operation()) {
                            case ast::BinaryOperationKind::Eq:
                                resultValue = cc().irBuilder().CreateFCmpOEQ(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::NotEq:
                                resultValue = cc().irBuilder().CreateFCmpONE(lValue, rValue);
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
                            case ast::BinaryOperationKind::GreaterThan:
                                resultValue = cc().irBuilder().CreateFCmpOGT(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::GreaterThanOrEqual:
                                resultValue = cc().irBuilder().CreateFCmpOGE(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::LessThan:
                                resultValue = cc().irBuilder().CreateFCmpOLT(lValue, rValue);
                                break;
                            case ast::BinaryOperationKind::LessThanOrEqual:
                                resultValue = cc().irBuilder().CreateFCmpOLE(lValue, rValue);
                                break;
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be an arithmetic operator)");
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

            virtual bool visitingWhileExpr(ast::WhileExpr *whileExpr) {

                //Prepare the BasicBlocks
                llvm::Function *currentFunc = cc().irBuilder().GetInsertBlock()->getParent();
                llvm::BasicBlock *conditionBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileCondition");
                llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileBody");
                llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileEnd");

                cc().irBuilder().CreateBr(conditionBlock);
                currentFunc->getBasicBlockList().push_back(conditionBlock);
                cc().irBuilder().SetInsertPoint(conditionBlock);

                //Emit the condition
                llvm::Value *condValue = emitExpr(whileExpr->condition(), cc());

                //If the condition is true branch to the body block, otherwise branch to the end block
                cc().irBuilder().CreateCondBr(condValue, bodyBlock, endBlock);

                //Emit the body block
                currentFunc->getBasicBlockList().push_back(bodyBlock);
                cc().irBuilder().SetInsertPoint(bodyBlock);
                emitExpr(whileExpr->body(), cc());
                //Loop back to the condition
                cc().irBuilder().CreateBr(conditionBlock);

                //Emit the end block
                currentFunc->getBasicBlockList().push_back(endBlock);
                cc().irBuilder().SetInsertPoint(endBlock);

                return false;
            }

            virtual bool visitingCompoundExpr(ast::CompoundExpr *expr) override {

                llvm::Value *lastValue = nullptr;
                for (ast::ExprStmt *expr : expr->expressions()) {
                    ExprAstVisitor exprAstVisitor{cc()};
                    expr->accept(&exprAstVisitor);
                    if(exprAstVisitor.hasValue()) {
                        lastValue = exprAstVisitor.llvmValue();
                    }
                }
                valueStack_.push(lastValue);

                return false;
            }
        };

        llvm::Value *emitExpr(ast::ExprStmt *exprStmt, CompileContext &cc) {
            ExprAstVisitor visitor{cc};
            exprStmt->accept(&visitor);

            return visitor.hasValue() ? visitor.llvmValue() : nullptr;
        }
    }
}