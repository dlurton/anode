
#include "compile.h"

#include "ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"

#pragma GCC diagnostic pop

using namespace lwnn::ast;

/**
 * The best way I have found to figure out what instructions to use with LLVM is to
 * write some c++ that does the equivalent of what want to do and compile it here: http://ellcc.org/demo/index.cgi
 * then examine the IR that's generated.
 */

namespace lwnn {
    namespace compile {

        const int ALIGNMENT = 4;


        llvm::Type *to_llvmType(type::PrimitiveType primitiveType, llvm::LLVMContext &llvmContext) {
            ASSERT(primitiveType != type::PrimitiveType::Void && "Expected a primitive data type here");
            switch (primitiveType) {
                case type::PrimitiveType::Void:
                    return llvm::Type::getVoidTy(llvmContext);
                case type::PrimitiveType::Bool:
                    return llvm::Type::getInt1Ty(llvmContext);
                case type::PrimitiveType::Int32:
                    return llvm::Type::getInt32Ty(llvmContext);
                case type::PrimitiveType::Float:
                    return llvm::Type::getFloatTy(llvmContext);
                case type::PrimitiveType::Double:
                    return llvm::Type::getDoubleTy(llvmContext);
                default:
                    ASSERT_FAIL("Unhandled PrimitiveType");
            }
        }

        llvm::Type *to_llvmType(type::Type *type, llvm::LLVMContext &llvmContext) {
            ASSERT(type);
            type::PrimitiveType primitiveType = type->primitiveType();
            return to_llvmType(primitiveType, llvmContext);
        }

        class ExprAstVisitor : public ScopeFollowingVisitor {
            llvm::LLVMContext &llvmContext_;
            llvm::IRBuilder<> irBuilder_;
            llvm::Module &llvmModule_;
            std::stack<llvm::Value *> valueStack_;

        public:
            ExprAstVisitor(llvm::LLVMContext &llvmContext, llvm::Module &llvmModule, llvm::IRBuilder<> irBuilder)
                : llvmContext_{llvmContext}, llvmModule_{llvmModule}, irBuilder_{irBuilder} { }

            bool hasValue() {
                return valueStack_.size() > 0;
            }

            llvm::Value *getValue() {
                ASSERT(valueStack_.size() == 1);
                return valueStack_.top();
            }

            virtual void visitedCastExpr(CastExpr *expr) {
                llvm::Value *value = valueStack_.top();
                valueStack_.pop();

                ASSERT(expr->type()->isPrimitive());
                ASSERT(expr->valueExpr()->type()->isPrimitive());

                llvm::Value* castedValue = nullptr;
                llvm::Type *destLlvmType = to_llvmType(expr->type(), llvmContext_);
                switch(expr->valueExpr()->type()->primitiveType()) {
                    //From int32
                    case type::PrimitiveType::Int32:
                        switch(expr->type()->primitiveType()) {
                            //To bool
                            case type::PrimitiveType::Bool: {
                                castedValue = irBuilder_.CreateICmpNE(value, getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            }
                            //To int
                            case type::PrimitiveType::Int32:
                                castedValue = value;
                                break;
                            //to float
                            case type::PrimitiveType::Float:
                                castedValue = irBuilder_.CreateSIToFP(value, destLlvmType);
                                break;
                            default:
                                ASSERT_FAIL("Unknown cast from int32");
                        }
                        break;
                    //From float
                    case type::PrimitiveType::Float:
                        switch(expr->type()->primitiveType()) {
                            //To to int32 or bool
                            case type::PrimitiveType::Bool:
                                castedValue = irBuilder_.CreateFCmpUNE(value, getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            case type::PrimitiveType::Int32:
                                castedValue = irBuilder_.CreateFPToSI(value, destLlvmType);
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

            virtual void visitedVariableDeclExpr(VariableDeclExpr *expr) override {
                llvm::Type *type = to_llvmType(expr->type(), llvmContext_);

                //For now assume global variables.
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                llvm::GlobalVariable *globalVariable = llvmModule_.getNamedGlobal(expr->name());
                ASSERT(globalVariable);
                globalVariable->setAlignment(ALIGNMENT);
                globalVariable->setInitializer(getDefaultValueForType(expr->type()));

                 visitVariableRefExpr(expr);
            }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *expr) override {
                valueStack_.push(llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, (uint64_t)expr->value(), true)));
            }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *expr) override {
                valueStack_.push(llvm::ConstantFP::get(llvmContext_, llvm::APFloat(expr->value())));
            }

            virtual void visitLiteralBoolExpr(LiteralBoolExpr *expr) {
                valueStack_.push(llvm::ConstantInt::get(llvmContext_, llvm::APInt(1, (uint64_t)expr->value(), true)));
            }

            virtual void visitVariableRefExpr(VariableRefExpr *expr) override {
                //We are assuming global variables for now
                //llvm::Value *allocaInst = lookupVariable(expr->name());

                llvm::Type *type = to_llvmType(expr->type(), llvmContext_);
                llvm::GlobalVariable *globalVar = llvmModule_.getNamedGlobal(expr->name());
                ASSERT(globalVar);

                if(expr->variableAccess() == VariableAccess::Write) {
                    valueStack_.push(globalVar);
                    return;
                }

                llvm::LoadInst *loadInst = irBuilder_.CreateLoad(globalVar);
                loadInst->setAlignment(ALIGNMENT);
                valueStack_.push(loadInst);

            }

            virtual void visitedBinaryExpr(BinaryExpr *expr) override {
                ASSERT(expr->lValue()->type() == expr->rValue()->type() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *result = createOperation(lValue, rValue, expr->operation(), expr->operandsType());
                valueStack_.push(result);
            }

            virtual void visitedSelectExpr(SelectExpr *) override {
                llvm::Value *falseValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *trueValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *condValue = valueStack_.top();
                valueStack_.pop();

                valueStack_.push(irBuilder_.CreateSelect(condValue, trueValue, falseValue));
            }

        private:

            llvm::Constant *getDefaultValueForType(type::Type *type) {
                ASSERT(type->isPrimitive());
                return getDefaultValueForType(type->primitiveType());
            }

            llvm::Constant *getDefaultValueForType(type::PrimitiveType primitiveType) {
                ASSERT(primitiveType != type::PrimitiveType::Void);
                switch(primitiveType) {
                    case type::PrimitiveType::Bool:
                        return llvm::ConstantInt::get(llvmContext_, llvm::APInt(1, 0, false));
                    case type::PrimitiveType::Int32:
                        return llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, 0, true));
                    case type::PrimitiveType::Float:
                        //APFloat has different constructors depending on if you want a float or a double...
                        return llvm::ConstantFP::get(llvmContext_, llvm::APFloat((float)0.0));
                    case type::PrimitiveType::Double:
                        return llvm::ConstantFP::get(llvmContext_, llvm::APFloat(0.0));
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            }

            llvm::Value *
            createOperation(llvm::Value *lValue, llvm::Value *rValue, BinaryOperationKind op, const type::Type *type) {
                ASSERT(type->isPrimitive() && "Only primitive types currently supported here");

                if(op == BinaryOperationKind::Assign) {
                    irBuilder_.CreateStore(rValue, lValue); //(args swapped on purpose)
                    //Note:  I think this will call rValue a second time if rValue is a call site.
                    return rValue;
                }
                switch (type->primitiveType()) {
                    case type::PrimitiveType::Bool:
                        switch(op) {
                            case BinaryOperationKind::Eq:
                                return irBuilder_.CreateICmpEQ(lValue, rValue);
//                            case BinaryOperationKind::And:
//                                return irBuilder_.CreateICmpEQ(lValue, rValue);
//                            case BinaryOperationKind::Or:
//                                return irBuilder_.CreateICmpEQ(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unsupported BinaryOperationKind for bool primitive type")
                        }
                    case type::PrimitiveType::Int32:
                        switch (op) {
                            case BinaryOperationKind::Eq:
                                return irBuilder_.CreateICmpEQ(lValue, rValue);
                            case BinaryOperationKind::Add:
                                return irBuilder_.CreateAdd(lValue, rValue);
                            case BinaryOperationKind::Sub:
                                return irBuilder_.CreateSub(lValue, rValue);
                            case BinaryOperationKind::Mul:
                                return irBuilder_.CreateMul(lValue, rValue);
                            case BinaryOperationKind::Div:
                                return irBuilder_.CreateSDiv(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                    case type::PrimitiveType::Float:
                        switch (op) {
                            case BinaryOperationKind::Eq:
                                return irBuilder_.CreateFCmpOEQ(lValue, rValue);
                            case BinaryOperationKind::Add:
                                return irBuilder_.CreateFAdd(lValue, rValue);
                            case BinaryOperationKind::Sub:
                                return irBuilder_.CreateFSub(lValue, rValue);
                            case BinaryOperationKind::Mul:
                                return irBuilder_.CreateFMul(lValue, rValue);
                            case BinaryOperationKind::Div:
                                return irBuilder_.CreateFDiv(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            }
        };


        class ModuleAstVisitor : public ScopeFollowingVisitor {
            llvm::LLVMContext &llvmContext_;
            llvm::TargetMachine &targetMachine_;
            llvm::IRBuilder<> irBuilder_;
            std::unique_ptr<llvm::Module> llvmModule_;
            llvm::Function *initFunc_;
            int resultExprStmtCount_ = 0;
            llvm::Function *resultFunc_;
            llvm::Value *executionContextPtrValue_;
        public:
            ModuleAstVisitor(llvm::LLVMContext &llvmContext, llvm::TargetMachine &targetMachine)
                : llvmContext_(llvmContext),
                  targetMachine_(targetMachine),
                  irBuilder_(llvmContext) { }

            std::unique_ptr<llvm::Module> surrenderModule() {
                return std::move(llvmModule_);
            }

            virtual bool visitingExprStmt(ExprStmt *expr) override {
                ScopeFollowingVisitor::visitingExprStmt(expr);
                ExprAstVisitor visitor{llvmContext_, *llvmModule_, irBuilder_};
                expr->accept(&visitor);

                if(visitor.hasValue()) {
                    resultExprStmtCount_++;
                    std::string variableName = string::format("result_%d", resultExprStmtCount_);
                    llvm::AllocaInst *resultVar = irBuilder_.CreateAlloca(visitor.getValue()->getType(), nullptr, variableName);

                    irBuilder_.CreateStore(visitor.getValue(), resultVar);

                    std::string bitcastVariableName = string::format("bitcasted_%d", resultExprStmtCount_);
                    auto bitcasted = irBuilder_.CreateBitCast(resultVar, llvm::Type::getInt64PtrTy(llvmContext_));
//
                    std::vector<llvm::Value*> args {
                        //ExecutionContext pointer
                        executionContextPtrValue_,
                        //PrimitiveType,
                        llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, (uint64_t)expr->type()->primitiveType(), true)),
                        //Pointer to value.
                        bitcasted
                    };
                    auto call = irBuilder_.CreateCall(resultFunc_, args);
                }

                return false;
            }

            virtual void visitingModule(Module *module) override {
                ScopeFollowingVisitor::visitingModule(module);

                llvmModule_ = llvm::make_unique<llvm::Module>(module->name(), llvmContext_);
                llvmModule_->setDataLayout(targetMachine_.createDataLayout());

                declareResultFunction();

                startModuleInitFunc(module);

                declareExecutionContextGlobal();

                //Define all the global variables now...
                //The reason for doing this here instead of in visitVariableDeclExpr is we do not
                //have VariableDeclExprs for the imported global variables.
                std::vector<scope::Symbol*> globals = module->scope()->symbols();
                for(scope::Symbol *symbol : globals) {
                    llvmModule_->getOrInsertGlobal(symbol->name(), to_llvmType(symbol->type(), llvmContext_));
                    llvm::GlobalVariable *globalVar = llvmModule_->getNamedGlobal(symbol->name());
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    globalVar->setAlignment(ALIGNMENT);
                }
            }

            virtual void visitedModule(Module *module) override {
                irBuilder_.CreateRetVoid();
                llvm::raw_ostream &os = llvm::errs();
                if(llvm::verifyModule(*llvmModule_.get(), &os)) {
                    ASSERT_FAIL();
               }
            }

        private:
            void startModuleInitFunc(const Module *module) {
                auto initFuncRetType = llvm::Type::getVoidTy(llvmContext_);
                initFunc_ = llvm::cast<llvm::Function>(
                    llvmModule_->getOrInsertFunction(module->name() + MODULE_INIT_SUFFIX, initFuncRetType));

                initFunc_->setCallingConv(llvm::CallingConv::C);
                auto initFuncBlock = llvm::BasicBlock::Create(llvmContext_, "begin", initFunc_);

                irBuilder_.SetInsertPoint(initFuncBlock);
            }

            void declareResultFunction() {
                resultFunc_ = llvm::cast<llvm::Function>(llvmModule_->getOrInsertFunction(
                    RECEIVE_RESULT_FUNC_NAME,
                    llvm::Type::getVoidTy(llvmContext_),        //Return type
                    llvm::Type::getInt64PtrTy(llvmContext_),    //Pointer to ExecutionContext
                    llvm::Type::getInt32Ty(llvmContext_),       //type::PrimitiveType
                    llvm::Type::getInt64PtrTy(llvmContext_)));  //Pointer to value


                auto paramItr = resultFunc_->arg_begin();
                llvm::Value *executionContext = paramItr++;
                executionContext->setName("executionContext");

                llvm::Value *primitiveType = paramItr++;
                primitiveType->setName("primitiveType");

                llvm::Value *valuePtr = paramItr++;
                valuePtr->setName("valuePtr");
            }


            void declareExecutionContextGlobal() {
                llvmModule_->getOrInsertGlobal(EXECUTION_CONTEXT_GLOBAL_NAME, llvm::Type::getInt64Ty(llvmContext_));
                llvm::GlobalVariable *globalVar = llvmModule_->getNamedGlobal(EXECUTION_CONTEXT_GLOBAL_NAME);
                globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                globalVar->setAlignment(ALIGNMENT);

                executionContextPtrValue_ = globalVar;
            }

        };

        std::unique_ptr<llvm::Module> generateCode(Module *module, llvm::LLVMContext &context, llvm::TargetMachine *targetMachine) {
            ModuleAstVisitor visitor{context, *targetMachine};
            module->accept(&visitor);
            return visitor.surrenderModule();
        }

    } //namespace compile
} //namespace lwnn