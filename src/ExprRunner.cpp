#include "ExprRunner.h"
#include "AstVisitor.h"
#include "AstWalker.h"
#include "PrettyPrinter.h"

#include <map>
#include <stack>

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

    class CodeGenVisitor : public AstVisitor {
        llvm::LLVMContext &context_;
        llvm::TargetMachine &targetMachine_;
        llvm::IRBuilder<> irBuilder_;
        std::unique_ptr<llvm::Module> module_;
        llvm::Function* function_;
        llvm::BasicBlock *block_;

        typedef std::unordered_map<std::string, llvm::AllocaInst*> AllocaScope;

        //this is a deque and not an actual std::stack because we need the ability to iterate over its contents
        std::deque<AllocaScope> allocaScopeStack_;
        std::stack<llvm::Value*> valueStack_;
        std::stack<const AstNode*> ancestryStack_;

    public:
        CodeGenVisitor(llvm::LLVMContext &context, llvm::TargetMachine &targetMachine)
                : context_{context}, targetMachine_{targetMachine}, irBuilder_{context} { }

        virtual void visitingModule(const Module *module) override {
            module_ = llvm::make_unique<llvm::Module>(module->name(), context_);
            module_->setDataLayout(targetMachine_.createDataLayout());
        }

        virtual void visitedModule(const Module *) override {
            DEBUG_ASSERT(valueStack_.size() == 0 && "When compilation complete, no values should remain.");
        }

        virtual void visitingFunction(const Function *func) override {
            std::vector<llvm::Type*> argTypes;

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
            if(ancestryStack_.size() >= 1
               && expr->nodeKind() == NodeKind::Block
               && valueStack_.size() > 0) {
                valueStack_.pop();
            }
        }

        llvm::Value *lookupVariable(const std::string &name) {
            for(auto scope = allocaScopeStack_.rbegin(); scope != allocaScopeStack_.rend(); ++scope) {
                auto foundValue = scope->find(name);
                if(foundValue != scope->end()) {
                    return foundValue->second;
                }
            }

            throw InvalidStateException(std::string("Variable '") + name + std::string("' was not defined."));
        }

        virtual void visitingBlock(const BlockExpr *expr) override {
            allocaScopeStack_.emplace_back();
            AllocaScope &topScope = allocaScopeStack_.back();

            for(auto var : expr->scope()->variables()) {
                if(topScope.find(var->name()) != topScope.end()) {
                    throw InvalidStateException("More than one variable named '" + var->name() +
                                                "' was defined in the current scope.");
                }

                llvm::Type *type{getType(var->dataType())};
                llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                topScope[var->name()] = allocaInst;
            }
        }

        llvm::Type *getType(DataType type)
        {
            switch(type) {
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

        llvm::Value *createOperation(llvm::Value *lValue, llvm::Value *rValue, OperationKind op, DataType dataType) {
            switch(dataType) {
                case DataType::Int32:
                    switch(op) {
                        case OperationKind::Add: return irBuilder_.CreateAdd(lValue, rValue);
                        case OperationKind::Sub: return irBuilder_.CreateSub(lValue, rValue);
                        case OperationKind::Mul: return irBuilder_.CreateMul(lValue, rValue);
                        case OperationKind::Div: return irBuilder_.CreateSDiv(lValue, rValue);
                        default:
                            throw UnhandledSwitchCase();
                    }
                case DataType::Float:
                    switch(op) {
                        case OperationKind::Add: return irBuilder_.CreateFAdd(lValue, rValue);
                        case OperationKind::Sub: return irBuilder_.CreateFSub(lValue, rValue);
                        case OperationKind::Mul: return irBuilder_.CreateFMul(lValue, rValue);
                        case OperationKind::Div: return irBuilder_.CreateFDiv(lValue, rValue);
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
            llvm::Value* retValue = valueStack_.top();
            valueStack_.pop();
            irBuilder_.CreateRet(retValue);
        }

    }; // class CodeGenVisitor


    //The llvm::orc::createResolver(...) version of this doesn't seem to work for some reason...
    template <typename DylibLookupFtorT, typename ExternalLookupFtorT>
    std::unique_ptr<llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT>>
    createLambdaResolver2(DylibLookupFtorT DylibLookupFtor, ExternalLookupFtorT ExternalLookupFtor) {
        typedef llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT> LR;
        return std::make_unique<LR>(DylibLookupFtor, ExternalLookupFtor);
    }

    /** This class originally taken from:
     * https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/include/KaleidoscopeJIT.h
     */
    class SimpleJIT {
    private:
        std::unique_ptr<llvm::TargetMachine> TM;
        const llvm::DataLayout DL;
        llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
        llvm::orc::IRCompileLayer<decltype(ObjectLayer), llvm::orc::SimpleCompiler> CompileLayer;

    public:
        using ModuleHandle = decltype(CompileLayer)::ModuleHandleT;

        SimpleJIT()
                : TM(llvm::EngineBuilder().selectTarget()), DL(TM->createDataLayout()),
                  ObjectLayer([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
                  CompileLayer(ObjectLayer, llvm::orc::SimpleCompiler(*TM)) {
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
        }

        llvm::TargetMachine &getTargetMachine() { return *TM; }

        ModuleHandle addModule(std::shared_ptr<llvm::Module> M) {
            // Build our symbol resolver:
            // Lambda 1: Look back into the JIT itself to find symbols that are part of
            //           the same "logical dylib".
            // Lambda 2: Search for external symbols in the host process.
            auto Resolver = createLambdaResolver2(
                    [&](const std::string &Name) {
                        if (auto Sym = CompileLayer.findSymbol(Name, false))
                            return Sym;
                        return llvm::JITSymbol(nullptr);
                    },
                    [](const std::string &Name) {
                        if (auto SymAddr =
                                llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
                            return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
                        return llvm::JITSymbol(nullptr);
                    });

            // Add the set to the JIT with the resolver we created above and a newly
            // created SectionMemoryManager.
            return CompileLayer.addModule(M, std::move(Resolver));
        }

        llvm::JITSymbol findSymbol(const std::string Name) {
            std::string MangledName;
            llvm::raw_string_ostream MangledNameStream(MangledName);
            llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
            return CompileLayer.findSymbol(MangledNameStream.str(), true);
        }

        void removeModule(ModuleHandle H) {
            CompileLayer.removeModule(H);
        }
    }; //SimpleJIT

    //TODO:  make this a PIMPL with a public interface
    class ExecutionContext {
        //TODO:  determine if definition order (destruction order) is still significant, and if so
        //update this note to say so.

        llvm::LLVMContext context_;
        std::unique_ptr<SimpleJIT> jit_ = std::make_unique<SimpleJIT>();

        static void prettyPrint(const Module *module) {
            lwnn::PrettyPrinterVisitor visitor{std::cout};
            AstWalker walker{&visitor};
            walker.walkTree(module);
        }

    public:

        uint64_t getSymbolAddress(const std::string &name) {
            llvm::JITSymbol symbol = jit_->findSymbol(name);

            if(!symbol)
                return 0;

            uint64_t retval = symbol.getAddress();
            return retval;
        }

        void addModule(const Module *module) {
            //prettyPrint(module);
            lwnn::CodeGenVisitor visitor{ context_, jit_->getTargetMachine()};
            AstWalker walker{&visitor};
            walker.walkTree(module);

            visitor.dumpIR();
            std::unique_ptr<llvm::Module> llvmModule = visitor.surrenderLlvmModule();
            jit_->addModule(move(llvmModule));
        }
    };

    namespace ExprRunner {
        typedef float (*FloatFuncPtr)(void);
        typedef int (*IntFuncPtr)(void);

        void init() {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }

        namespace {
            const std::string FUNC_NAME = "exprFunc";

            std::unique_ptr<ExecutionContext> makeExecutionContext(std::unique_ptr<const Module> module) {
                auto ec = make_unique<ExecutionContext>();
                ec->addModule(module.get());
                return ec;
            }

            std::unique_ptr<ExecutionContext> makeExecutionContext(std::unique_ptr<const Expr> expr) {
                FunctionBuilder fb{SourceSpan::Any, FUNC_NAME, expr->dataType()};
                BlockExprBuilder &bb = fb.blockBuilder();
                bb.addExpression(move(expr));

                ModuleBuilder mb{"ExprModule"};
                mb.addFunction(fb.build());
                std::unique_ptr<const Module> module = mb.build();

                return makeExecutionContext(std::move(module));
            }
        }

        std::string compile(std::unique_ptr<const Expr> expr) {
            const char *FUNC_NAME = "someFunc";
            DataType exprDataType = expr->dataType();

            std::unique_ptr<const Return> retExpr = std::make_unique<Return>(SourceSpan::Any, std::move(expr));
            FunctionBuilder fb{ SourceSpan::Any, FUNC_NAME, retExpr->dataType() };

            BlockExprBuilder &bb = fb.blockBuilder();

            bb.addExpression(std::move(retExpr));

            ModuleBuilder mb{"someModule"};

            mb.addFunction(fb.build());

            std::unique_ptr<const Module> m{mb.build()};

            PrettyPrinterVisitor prettyPrinterVisitor(std::cout);
            AstWalker prettyPrinterWalker(&prettyPrinterVisitor);
            prettyPrinterWalker.walkModule(m.get());
            std::cout << "\n";

//            llvm::LLVMContext ctx;
//            auto tm = std::unique_ptr<llvm::TargetMachine>(llvm::EngineBuilder().selectTarget());
//            lwnn::CodeGenVisitor codeGenVisitor{ctx, *tm.get()};
//            AstWalker codeGenWalker{&codeGenVisitor};
//            codeGenWalker.walkModule(m.get());

            //codeGenVisitor.dumpIR();

            ExecutionContext ec;
            //const std::unique_ptr<llvm::Module> &module = codeGenVisitor.surrenderLlvmModule();
            ec.addModule(m.get());
            
            uint64_t funcPtr = ec.getSymbolAddress(FUNC_NAME);

            std::string resultAsString;
            switch(exprDataType) {
                case DataType::Int32: {
                    IntFuncPtr intFuncPtr = reinterpret_cast<IntFuncPtr>(funcPtr);
                    int result = intFuncPtr();
                    resultAsString = std::to_string(result);
                    break;
                }
                default:
                    throw UnhandledSwitchCase();
            }

            return resultAsString;
        }

        float runFloatExpr(std::unique_ptr<const Expr> expr) {

            if(expr->dataType() != DataType::Float) {
                throw FatalException("expr->dataType() != DataType::Float");
            }

            std::unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto funcPtr = reinterpret_cast<FloatFuncPtr>(ec->getSymbolAddress(FUNC_NAME));
            float retval = funcPtr();
            return retval;
        }

        int runInt32Expr(std::unique_ptr<const Expr> expr) {

            if(expr->dataType() != DataType::Int32) {
                throw FatalException("expr->dataType() != DataType::Int32");
            }

            std::unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto funcPtr = reinterpret_cast<IntFuncPtr>(ec->getSymbolAddress(FUNC_NAME));
            int retval = funcPtr();
            return retval;
        }

        std::string execute(std::unique_ptr<const Expr> expr);
    } //namespace ExprRunner
} //namespace float
