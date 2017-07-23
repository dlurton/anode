
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

        const int ALIGNMENT = 4;
        /** Base class to generate IR for specific types. */
        class CodeGenHelper {
        public:
            /** Returns equivalent of 0 or null.*/
            virtual llvm::Constant *getDefaultValue() = 0;

            /** Creates arithmatic operation. */
            virtual llvm::Value *createOperation(llvm::Value *lValue, llvm::Value *rValue, BinaryOperationKind op) = 0;

            /** Returns the LLVMM type. */
            virtual llvm::Type *getLlvmType() = 0;

            /** Creates a cast to a different type. */
            virtual llvm::Value *createCastTo(llvm::Value *, llvm::Type *) = 0;

        };

        class Int32CodeGenHelper : public CodeGenHelper {
            llvm::LLVMContext &llvmContext_;
            llvm::IRBuilder<> &irBuilder_;
        public:
            Int32CodeGenHelper(llvm::LLVMContext &llvmContext, llvm::IRBuilder<> &irBuilder)
                : llvmContext_{llvmContext}, irBuilder_{irBuilder} { }

            virtual llvm::Constant *getDefaultValue() {
                return llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, 0, true));
            }

            /** Creates arithmetic operation. */
            virtual llvm::Value *createOperation(llvm::Value *lValue, llvm::Value *rValue, BinaryOperationKind op) {
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
            };

            /** Returns the LLVM type. */
            virtual llvm::Type *getLlvmType() {
                return llvm::Type::getInt32Ty(llvmContext_);
            };

            /** Creates a cast to a different type. */
            virtual llvm::Value *createCastTo(llvm::Value *value, llvm::Type *type) {
                if(type->isFloatTy()) {
                    return irBuilder_.CreateSIToFP(value, type);
                }

                ASSERT_FAIL("Can't cast to requested type");
            }
        }; //class Int32CodeGenHelper

        class FloatCodeGenHelper : public CodeGenHelper {
            llvm::LLVMContext &llvmContext_;
            llvm::IRBuilder<> &irBuilder_;
        public:
            FloatCodeGenHelper(llvm::LLVMContext &llvmContext, llvm::IRBuilder<> &irBuilder)
                : llvmContext_{llvmContext}, irBuilder_{irBuilder} { }

            virtual llvm::Constant *getDefaultValue() {
                return llvm::ConstantFP::get(llvmContext_, llvm::APFloat((float)0));
            }

            /** Creates arithmetic operation. */
            virtual llvm::Value *createOperation(llvm::Value *lValue, llvm::Value *rValue, BinaryOperationKind op) {
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
            };

            /** Returns the LLVMM type. */
            virtual llvm::Type *getLlvmType() {
                return llvm::Type::getFloatTy(llvmContext_);
            };

            /** Creates a cast to a different type. */
            virtual llvm::Value *createCastTo(llvm::Value *value, llvm::Type *type) {
                if(type->isIntegerTy()) {
                    return irBuilder_.CreateFPToSI(value, type);
                }

                ASSERT_FAIL("Can't cast to requested type");
            }
        }; //class FloatCodeGenHelper

        class CodeGenerationContext {
        private:
            Int32CodeGenHelper int32CodeGenHelper_;
            FloatCodeGenHelper floatCodeGenHelper_;

        public:
            llvm::LLVMContext &llvmContext;
            llvm::IRBuilder<> &irBuilder;
            llvm::Module &llvmModule;

            CodeGenerationContext(
                llvm::LLVMContext &llvmContext,
                llvm::Module &module,
                llvm::IRBuilder<> &irBuilder)
                : llvmContext{llvmContext},
                  llvmModule{module},
                  irBuilder{irBuilder},
                  int32CodeGenHelper_{llvmContext, irBuilder},
                  floatCodeGenHelper_{llvmContext, irBuilder} { }

        public:
            CodeGenHelper *getCodeGenHelper(type::Type *type) {
                ASSERT(type);
                ASSERT(type->isPrimitive() && "Expected a primitive data type here");
                ASSERT(type->primitiveType() != type::PrimitiveType::Void);

                switch (type->primitiveType()) {
                    case type::PrimitiveType::Int32:
                        return &int32CodeGenHelper_;
                    case type::PrimitiveType::Float:
                        return &floatCodeGenHelper_;
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            } //getCodeGenHelper

        }; //CodeGenenerationContext

        class ExprAstVisitor : public ScopeFollowingVisitor {
            CodeGenerationContext &context_;
            std::stack<llvm::Value *> valueStack_;

        public:
            ExprAstVisitor(CodeGenerationContext &context)
                : context_(context) { }

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
                CodeGenHelper *fromIrGen = context_.getCodeGenHelper(expr->valueExpr()->type());
                CodeGenHelper *toIrGen = context_.getCodeGenHelper(expr->type());
                llvm::Value *castedValue = fromIrGen->createCastTo(value, toIrGen->getLlvmType());
                valueStack_.push(castedValue);
            }

            virtual void visitedVariableDeclExpr(VariableDeclExpr *expr) override {
                CodeGenHelper *typeHelper = context_.getCodeGenHelper(expr->type());
                llvm::Type *type = typeHelper->getLlvmType();

                //For now assume global variables.
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                llvm::GlobalVariable *globalVariable = context_.llvmModule.getNamedGlobal(expr->name());
                globalVariable->setAlignment(ALIGNMENT);
                globalVariable->setInitializer(typeHelper->getDefaultValue());

                if(expr->initializerExpr()) {
                    llvm::Value *initializer = valueStack_.top();
                    valueStack_.pop();
                    llvm::StoreInst *storeInst = context_.irBuilder.CreateStore(initializer, globalVariable);
                    storeInst->setAlignment(ALIGNMENT);
                }

                llvm::LoadInst *loadInst = context_.irBuilder.CreateLoad(globalVariable);
                loadInst->setAlignment(ALIGNMENT);
                valueStack_.push(loadInst);
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

                CodeGenHelper *helper = context_.getCodeGenHelper(expr->type());
                llvm::Type *type = helper->getLlvmType();
                llvm::GlobalVariable *globalVar = context_.llvmModule.getNamedGlobal(expr->name());
                ASSERT(globalVar);
                llvm::LoadInst *loadInst = context_.irBuilder.CreateLoad(globalVar);
                loadInst->setAlignment(ALIGNMENT);
                valueStack_.push(loadInst);

            }

            virtual void visitedBinaryExpr(BinaryExpr *expr) override {
                ASSERT(expr->lValue()->type() == expr->rValue()->type() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                auto typeHelper = context_.getCodeGenHelper(expr->type());
                llvm::Value *result = typeHelper->createOperation(lValue, rValue, expr->operation());
                valueStack_.push(result);
            }

        private:

            llvm::Value *getConstantInt32(int value) {
                llvm::ConstantInt *constantInt = llvm::ConstantInt::get(context_.llvmContext, llvm::APInt(32, value, true));
                return constantInt;
            }

            llvm::Value *getConstantFloat(float value) {
                return llvm::ConstantFP::get(context_.llvmContext, llvm::APFloat(value));
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
                ScopeFollowingVisitor::visitingExprStmt(expr);
                CodeGenerationContext cgc{llvmContext_, *llvmModule_.get(), irBuilder_ };
                ExprAstVisitor visitor{cgc};
                expr->accept(&visitor);

                if(visitor.hasValue()) {
                    //This is temporary - can't be returning in the middle of module init!
                    irBuilder_.CreateRet(visitor.getValue());
                }

                return false;
            }

            virtual void visitingModule(Module *module) override {
                ScopeFollowingVisitor::visitingModule(module);

                CodeGenerationContext cgc{llvmContext_, *llvmModule_.get(), irBuilder_ };

                llvmModule_ = llvm::make_unique<llvm::Module>(module->name(), llvmContext_);
                llvmModule_->setDataLayout(targetMachine_.createDataLayout());

                //The module init func has a return type of void if module->body() is not an ExprStmt.
                auto initFuncRetType = llvm::Type::getVoidTy(llvmContext_);
                if(module->body()->stmtKind() == StmtKind::ExprStmt) {
                    auto bodyExpr = static_cast<ExprStmt*>(module->body());
                    initFuncRetType = cgc.getCodeGenHelper(bodyExpr->type())->getLlvmType();
                    //to_llvmType(bodyExpr->type(), llvmContext_);
                }
                initFunc_ = llvm::cast<llvm::Function>(
                    llvmModule_->getOrInsertFunction(module->name() + MODULE_INIT_SUFFIX, initFuncRetType));

                initFunc_->setCallingConv(llvm::CallingConv::C);
                auto initFuncBlock = llvm::BasicBlock::Create(llvmContext_, "EntryBlock", initFunc_);

                irBuilder_.SetInsertPoint(initFuncBlock);

                //Define all the global variables now...
                std::vector<scope::Symbol*> globals = module->scope()->symbols();
                for(scope::Symbol *symbol : globals) {
                    auto codeGenHelper = cgc.getCodeGenHelper(symbol->type());
                    llvmModule_->getOrInsertGlobal(symbol->name(), codeGenHelper->getLlvmType());
                    llvm::GlobalVariable *globalVar = llvmModule_->getNamedGlobal(symbol->name());
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    globalVar->setAlignment(ALIGNMENT);
                }
            }
        };

        std::unique_ptr<llvm::Module> generateCode(Module *module, llvm::LLVMContext &context, llvm::TargetMachine *targetMachine) {
            ModuleAstVisitor visitor{context, *targetMachine};
            module->accept(&visitor);
            return visitor.surrenderModule();
        }

    } //namespace compile
} //namespace lwnn