#include "execute/execute.h"
#include "back/compile.h"
#include "front/ast_passes.h"
#include "front/visualize.h"

#include "LwnnJit.h"

#include <stack>
#include <vector>

namespace lwnn {
    namespace execute {
        /** This function is called by the JITd code to deliver the result of an expression */
        extern "C" void receiveReplResult(uint64_t ecPtr, type::PrimitiveType primitiveType, uint64_t valuePtr);


        class ExecutionContextImpl : public ExecutionContext {
            llvm::LLVMContext context_;
            std::unique_ptr<LwnnJit> jit_ = std::make_unique<LwnnJit>();
            bool dumpIROnModuleLoad_ = false;
            bool setPrettyPrintAst_ = false;
            std::vector<std::shared_ptr<ast::GlobalExportSymbol>> exportedSymbols_;
            ResultCallbackFunctor resultFunctor_ = nullptr;
        public:
            ExecutionContextImpl() {
                jit_->putExport(back::EXECUTION_CONTEXT_GLOBAL_NAME, (uint64_t)this);
                jit_->setEnableOptimization(false);
                jit_->putExport(back::RECEIVE_RESULT_FUNC_NAME, (uint64_t)receiveReplResult);

            }

            void dispatchResult(type::PrimitiveType primitiveType, uint64_t valuePtr) {
                if(resultFunctor_)
                    resultFunctor_(this, primitiveType, valuePtr);
            }

            uint64_t getSymbolAddress(const std::string &name) override {
                llvm::JITSymbol symbol = jit_->findSymbol(name);

                if (!symbol)
                    return 0;

                uint64_t retval = llvm::cantFail(symbol.getAddress());
                return retval;
            }

            virtual void setDumpIROnLoad(bool value) override {
                dumpIROnModuleLoad_ = value;
            }

            virtual void setPrettyPrintAst(bool value) override {
                setPrettyPrintAst_ = value;
            }

            virtual void setResultCallback(ResultCallbackFunctor functor)  {
                resultFunctor_ = functor;
            }

            virtual void prepareModule(ast::Module *module) override {

                for(auto symbolToImport : exportedSymbols_) {
                    module->scope()->addSymbol(symbolToImport.get());
                }

                error::ErrorStream errorStream {std::cerr};
                ast_passes::runAllPasses(module, errorStream);

                if(errorStream.errorCount() > 0) {
                    throw ExecutionException(
                        string::format("There were %d compilation errors.  See stderr for details.", errorStream.errorCount()));
                }

                for(auto symbolToExport : module->scope()->symbols()) {
                    if(dynamic_cast<ast::GlobalExportSymbol*>(symbolToExport) == nullptr) {
                        exportedSymbols_.emplace_back(
                            std::make_shared<ast::GlobalExportSymbol>(symbolToExport->name(), symbolToExport->type()));
                    }
                }

                if(setPrettyPrintAst_) {
                    visualize::prettyPrint(module);
                }
            }

        protected:
            virtual uint64_t loadModule(std::unique_ptr<ast::Module> module) override {
                ASSERT(module);

                std::unique_ptr<llvm::Module> llvmModule = back::emitModule(module.get(), context_, jit_->getTargetMachine());

                if(dumpIROnModuleLoad_) {
                    llvm::outs() << "LLVM IR:\n";
                    llvmModule->print(llvm::outs(), nullptr);
                }

                jit_->addModule(move(llvmModule));

                if(auto moduleInitSymbol = jit_->findSymbol(module->name() + back::MODULE_INIT_SUFFIX))
                    return llvm::cantFail(moduleInitSymbol.getAddress());

                return 0;
            }
        }; //ExecutionContextImpl

        extern "C" void receiveReplResult(uint64_t ecPtr, type::PrimitiveType primitiveType, uint64_t valuePtr) {
            ExecutionContextImpl *ec = reinterpret_cast<ExecutionContextImpl*>(ecPtr);
            ec->dispatchResult(primitiveType, valuePtr);
        }

        std::unique_ptr<ExecutionContext> createExecutionContext() {

            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();

            return std::make_unique<ExecutionContextImpl>();
        }
    } //namespace execute
} //namespace lwnn
