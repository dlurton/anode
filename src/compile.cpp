
#include "compile.h"

#include "ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"

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

namespace lwnn {
    namespace compile {

        llvm::Type *to_llvmType(type::Type *type, llvm::LLVMContext &llvmContext) {
            ASSERT(type);
            ASSERT(type->isPrimitive() && "Expected a primitive data type here");

            switch (type->primitiveType()) {
                case type::PrimitiveType::Void:
                    return llvm::Type::getVoidTy(llvmContext);
                case type::PrimitiveType::Bool:
                    return llvm::Type::getInt8Ty(llvmContext);
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


        class ExprAstVisitor : public ScopeFollowingVisitor {
            llvm::LLVMContext &llvmContext_;
            llvm::Module &llvmModule_;
            llvm::IRBuilder<> &irBuilder_;

            std::stack<llvm::Value *> valueStack_;

        public:
            ExprAstVisitor(llvm::LLVMContext &llvmContext_, llvm::Module &llvmModule, llvm::IRBuilder<> &irBuilder)
                : llvmContext_(llvmContext_), llvmModule_(llvmModule), irBuilder_(irBuilder) { }

            bool hasValue() {
                return valueStack_.size() > 0;
            }

            llvm::Value *getValue() {
                ASSERT(valueStack_.size() == 1);
                return valueStack_.top();
            }

            virtual bool visitingVariableDeclExpr(VariableDeclExpr *var) override {
                return true;
            }

            virtual void visitedVariableDeclExpr(VariableDeclExpr *expr) override {
                llvm::Type *type = to_llvmType(expr->type(), llvmContext_);

                //For now assume global variables.
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                llvmModule_.getOrInsertGlobal(expr->name(), type);

                llvm::Value *initializer = nullptr;
                if(expr->initializerExpr()) {
                    initializer = valueStack_.top();
                    valueStack_.pop();
                }

                llvm::GlobalVariable *globalVariable = llvmModule_.getNamedGlobal(expr->name());
                globalVariable->setLinkage(llvm::GlobalValue::CommonLinkage);
                globalVariable->setAlignment(4);
                globalVariable->setInitializer(getDefaultValueForPrimitiveType(expr->type()->primitiveType()));


                if(initializer) {
                    irBuilder_.CreateStore(initializer, globalVariable);
                }
                valueStack_.push(irBuilder_.CreateLoad(globalVariable));
            }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *expr) override {
                valueStack_.push(getConstantInt32(expr->value()));
            }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *expr) override {
                valueStack_.push(getConstantFloat(expr->value()));
            }

            virtual void visitVariableRefExpr(VariableRefExpr *expr) override {
                //We are assuming global variables for now
                //llvm::Value *allocaInst = lookupVariable(expr->name());

                llvm::Type *type = to_llvmType(expr->type(), llvmContext_);
                llvm::GlobalVariable *globalVar = llvmModule_.getNamedGlobal(expr->name());
                valueStack_.push(irBuilder_.CreateLoad(globalVar));
            }

            virtual void visitedBinaryExpr(BinaryExpr *expr) override {
                ASSERT(expr->lValue()->type() == expr->rValue()->type() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *result = createOperation(lValue, rValue, expr->operation(), expr->type());
                valueStack_.push(result);
            }

        private:
            llvm::Value *getConstantInt32(int value) {
                llvm::ConstantInt *constantInt = llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, value, true));
                return constantInt;
            }

            llvm::Value *getConstantFloat(float value) {
                return llvm::ConstantFP::get(llvmContext_, llvm::APFloat(value));
            }

            llvm::Constant *getDefaultValueForPrimitiveType(type::PrimitiveType primitiveType) {
                ASSERT(primitiveType != type::PrimitiveType::NotAPrimitive);
                ASSERT(primitiveType != type::PrimitiveType::Void);

                switch (primitiveType) {
                    case type::PrimitiveType::Bool:
                        return llvm::ConstantInt::get(llvmContext_, llvm::APInt(8, 1, true));
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
                switch (type->primitiveType()) {
                    case type::PrimitiveType::Int32:
                        switch (op) {
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
        public:
            ModuleAstVisitor(llvm::LLVMContext &llvmContext, llvm::TargetMachine &targetMachine)
                : llvmContext_(llvmContext),
                  targetMachine_(targetMachine),
                  irBuilder_(llvmContext) { }

            std::unique_ptr<llvm::Module> surrenderModule() {
                return std::move(llvmModule_);
            }

            virtual bool visitingExprStmt(ExprStmt *expr) override {
                ExprAstVisitor visitor{llvmContext_, *llvmModule_, irBuilder_ };
                expr->accept(&visitor);

                if(visitor.hasValue()) {
                    //This is temporary - can't be returning in the middle of module init!
                    irBuilder_.CreateRet(visitor.getValue());
                }

                return false;
            }

            virtual bool visitingModule(Module *module) override {
                llvmModule_ = llvm::make_unique<llvm::Module>(module->name(), llvmContext_);
                llvmModule_->setDataLayout(targetMachine_.createDataLayout());

                //The module init func has a return type of void if module->body() is not an ExprStmt.
                auto initFuncRetType = llvm::Type::getVoidTy(llvmContext_);
                if(module->body()->stmtKind() == StmtKind::ExprStmt) {
                    auto bodyExpr = static_cast<ExprStmt*>(module->body());
                    initFuncRetType = to_llvmType(bodyExpr->type(), llvmContext_);
                }
                initFunc_ = llvm::cast<llvm::Function>(
                    llvmModule_->getOrInsertFunction(MODULE_INIT_FUNC_NAME, initFuncRetType));

                auto initFuncBlock = llvm::BasicBlock::Create(llvmContext_, "EntryBlock", initFunc_);

                irBuilder_.SetInsertPoint(initFuncBlock);

                return true;
            }

            virtual void visitedModule(Module *module) override {

            }
        };

        std::unique_ptr<llvm::Module> generateCode(Module *module, llvm::LLVMContext &context, llvm::TargetMachine *targetMachine) {
            ModuleAstVisitor visitor{context, *targetMachine};
            module->accept(&visitor);
            return visitor.surrenderModule();
        }

    } //namespace compile
} //namespace lwnn`