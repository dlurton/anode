
#include "CodeGenVisitor.h"

#include "AstWalker.h"

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

namespace lwnn {
    namespace compile {

        class CodeGenVisitor : public AstVisitor {
            llvm::LLVMContext &context_;
            llvm::TargetMachine &targetMachine_;
            llvm::IRBuilder<> irBuilder_;
            std::unique_ptr<llvm::Module> module_;
            llvm::Function *function_;
            llvm::BasicBlock *block_;

            typedef std::unordered_map<std::string, llvm::AllocaInst *> AllocaScope;

            //this is a deque and not an actual std::stack because we need the ability to iterate over its contents
            std::deque<AllocaScope> allocaScopeStack_;
            std::stack<llvm::Value *> valueStack_;
            std::stack<const AstNode *> ancestryStack_;

        public:
            CodeGenVisitor(llvm::LLVMContext &context, llvm::TargetMachine &targetMachine)
                : context_{ context }, targetMachine_{ targetMachine }, irBuilder_{ context } {}

            virtual void visitingModule(const Module *module) override {
                module_ = llvm::make_unique<llvm::Module>(module->name(), context_);
                module_->setDataLayout(targetMachine_.createDataLayout());
            }

            virtual void visitedModule(const Module *) override {
                DEBUG_ASSERT(valueStack_.size() == 0 && "When compilation complete, no values should remain.");
            }

            virtual void visitingFunction(const Function *func) override {
                std::vector<llvm::Type *> argTypes;

                function_ = llvm::cast<llvm::Function>(
                    module_->getOrInsertFunction(func->name(),
                                                 getType(func->returnType())));

                block_ = llvm::BasicBlock::Create(context_, "functionBody", function_);
                irBuilder_.SetInsertPoint(block_);
            }

            void dumpIR() {
                ASSERT_NOT_NULL(module_);
                std::cout << "LLVM IL:\n";
                module_->print(llvm::outs(), nullptr);
            }

        public:

            std::unique_ptr<llvm::Module> surrenderLlvmModule() {
                return std::move(module_);
            }

            virtual void visitingNode(const AstNode *expr) override {
                ancestryStack_.push(expr);
            }

            virtual void visitedNode(const AstNode *expr) override {
                DEBUG_ASSERT(ancestryStack_.top() == expr && "Top node of ancestryStack_ should be the current node.");
                ancestryStack_.pop();

                //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
                //sh7ould be removed.  (This is a consequence of "everything is an expression.")
                if (ancestryStack_.size() >= 1
                    && expr->nodeKind() == NodeKind::Block
                    && valueStack_.size() > 0) {
                    valueStack_.pop();
                }
            }

            llvm::Value *lookupVariable(const std::string &name) {
                for (auto scope = allocaScopeStack_.rbegin(); scope != allocaScopeStack_.rend(); ++scope) {
                    auto foundValue = scope->find(name);
                    if (foundValue != scope->end()) {
                        return foundValue->second;
                    }
                }

                throw InvalidStateException(std::string("Variable '") + name + std::string("' was not defined."));
            }

            virtual void visitingBlock(const BlockExpr *expr) override {
                allocaScopeStack_.emplace_back();
                AllocaScope &topScope = allocaScopeStack_.back();

                for (auto var : expr->scope()->variables()) {
                    if (topScope.find(var->name()) != topScope.end()) {
                        throw InvalidStateException("More than one variable named '" + var->name() +
                                                    "' was defined in the current scope.");
                    }

                    llvm::Type *type{ getType(var->dataType()) };
                    llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                    topScope[var->name()] = allocaInst;
                }
            }

            llvm::Type *getType(DataType type) {
                switch (type) {
                    case DataType::Void:
                        return llvm::Type::getVoidTy(context_);
                    case DataType::Bool:
                        return llvm::Type::getInt8Ty(context_);
                    case DataType::Int32:
                        return llvm::Type::getInt32Ty(context_);
                    case DataType::Float:
                        return llvm::Type::getFloatTy(context_);
                    case DataType::Double:
                        return llvm::Type::getDoubleTy(context_);
                    default:
                        throw UnhandledSwitchCase();
                }
            }

            virtual void visitedBlock(const BlockExpr *) override {
                allocaScopeStack_.pop_back();
            }

            virtual void visitedAssignVariable(const AssignVariable *expr) override {
                llvm::Value *inst = lookupVariable(expr->name());

                llvm::Value *value = valueStack_.top();
                valueStack_.pop();

                value = irBuilder_.CreateStore(value, inst);
                valueStack_.push(value);
            }

            virtual void visitedBinary(const BinaryExpr *expr) override {
                DEBUG_ASSERT(expr->lValue()->dataType() != expr->rValue()->dataType() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *result = createOperation(lValue, rValue, expr->operation(), expr->dataType());
                valueStack_.push(result);
            }

            llvm::Value *
            createOperation(llvm::Value *lValue, llvm::Value *rValue, OperationKind op, DataType dataType) {
                switch (dataType) {
                    case DataType::Int32:
                        switch (op) {
                            case OperationKind::Add:
                                return irBuilder_.CreateAdd(lValue, rValue);
                            case OperationKind::Sub:
                                return irBuilder_.CreateSub(lValue, rValue);
                            case OperationKind::Mul:
                                return irBuilder_.CreateMul(lValue, rValue);
                            case OperationKind::Div:
                                return irBuilder_.CreateSDiv(lValue, rValue);
                            default:
                                throw UnhandledSwitchCase();
                        }
                    case DataType::Float:
                        switch (op) {
                            case OperationKind::Add:
                                return irBuilder_.CreateFAdd(lValue, rValue);
                            case OperationKind::Sub:
                                return irBuilder_.CreateFSub(lValue, rValue);
                            case OperationKind::Mul:
                                return irBuilder_.CreateFMul(lValue, rValue);
                            case OperationKind::Div:
                                return irBuilder_.CreateFDiv(lValue, rValue);
                            default:
                                throw UnhandledSwitchCase();
                        }
                    default:
                        throw UnhandledSwitchCase();
                }
            }

            virtual void visitLiteralInt32(const LiteralInt32Expr *expr) override {
                valueStack_.push(getConstantInt32(expr->value()));
            }

            virtual void visitLiteralFloat(const LiteralFloatExpr *expr) override {
                valueStack_.push(getConstantFloat(expr->value()));
            }

            llvm::Value *getConstantInt32(int value) {
                llvm::ConstantInt *constantInt = llvm::ConstantInt::get(context_, llvm::APInt(32, value, true));
                return constantInt;
            }

            llvm::Value *getConstantFloat(float value) {
                return llvm::ConstantFP::get(context_, llvm::APFloat(value));
            }

            virtual void visitVariableRef(const VariableRef *expr) override {
                llvm::Value *allocaInst = lookupVariable(expr->name());
                valueStack_.push(irBuilder_.CreateLoad(allocaInst));
            }

            void visitedReturn(const Return *) override {
                DEBUG_ASSERT(valueStack_.size() > 0)
                llvm::Value *retValue = valueStack_.top();
                valueStack_.pop();
                irBuilder_.CreateRet(retValue);
            }

        }; // class CodeGenVisitor

        std::unique_ptr<llvm::Module> generateCode(const Module *lwnnModule, llvm::LLVMContext *context, llvm::TargetMachine *targetMachine) {
            CodeGenVisitor visitor{ *context, *targetMachine };
            AstWalker walker{ &visitor };
            walker.walkTree(lwnnModule);
            return visitor.surrenderLlvmModule();
        }
    } //namespace compile
} //namespace lwnn