#include "execute/execute.h"
#include "back/compile.h"
#include "front/ast_passes.h"
#include "front/visualize.h"
#include "runtime/builtins.h"

#include "AnodeJit.h"


namespace anode { namespace execute {
/** This function is called by the JITd code to deliver the result of an expression entered at the REPL. */
extern "C" void receiveReplResult(uint64_t ecPtr, type::PrimitiveType primitiveType, void *valuePtr);


/** This class contains contain pointers to garbage collected objects so it *must* inherit from gc or gc_cleanup
 * so that it's memory is scanned for  pointers to live objects, otherwise these may get collected prematurely. */
class ExecutionContextImpl : public ExecutionContext, public gc_cleanup, no_copy, no_assign {
    llvm::LLVMContext context_;
    std::unique_ptr<AnodeJit> jit_ = std::make_unique<AnodeJit>();
    bool dumpIROnModuleLoad_ = false;
    bool setPrettyPrintAst_ = false;
    gc_unordered_map<std::string, scope::Symbol*> exportedSymbols_;
    ResultCallbackFunctor resultFunctor_ = nullptr;
    back::TypeMap typeMap_;
public:
    ExecutionContextImpl() : typeMap_{context_} {
        jit_->putExport(back::EXECUTION_CONTEXT_GLOBAL_NAME, this);
        jit_->setEnableOptimization(false);
        jit_->putExport(back::RECEIVE_RESULT_FUNC_NAME, (void*)receiveReplResult);

        auto builtins = anode::runtime::getBuiltins();
        for(auto &pair : builtins) {
            jit_->putExport(pair.first, pair.second);
        }
    }

    void dispatchResult(type::PrimitiveType primitiveType, void *valuePtr) {
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

        for(auto &symbolToImport : exportedSymbols_) {
            symbolToImport.second->setIsExternal(true);
            module->scope()->addSymbol(symbolToImport.second);
        }

        error::ErrorStream errorStream {std::cerr};
        anode::front::passes::runAllPasses(module, errorStream);

        if(errorStream.errorCount() > 0) {
            throw ExecutionException(
                string::format("There were %d compilation errors.  See stderr for details.", errorStream.errorCount()));
        }

        if(setPrettyPrintAst_) {
            visualize::prettyPrint(module);
            std::cout.flush();
        }

        for(auto symbolToExport : module->scope()->symbols()) {
            auto found = exportedSymbols_.find(symbolToExport->fullyQualifiedName());
            if(found == exportedSymbols_.end()) {
                exportedSymbols_.emplace(symbolToExport->name(), symbolToExport);
            }
        }
    }

protected:
    virtual uint64_t loadModule(ast::Module *module) override {
        ASSERT(module);

        std::unique_ptr<llvm::Module> llvmModule = back::emitModule(module, typeMap_, context_, jit_->getTargetMachine());

        if(dumpIROnModuleLoad_) {
            std::cerr << "LLVM IR:\n";
#ifdef ANODE_DEBUG
            llvmModule->dump();
#endif
        }

        jit_->addModule(move(llvmModule));

        if(auto moduleInitSymbol = jit_->findSymbol(module->name() + back::MODULE_INIT_SUFFIX))
            return llvm::cantFail(moduleInitSymbol.getAddress());

        return 0;
    }
}; //ExecutionContextImpl

extern "C" void receiveReplResult(uint64_t ecPtr, type::PrimitiveType primitiveType, void *valuePtr) {
    ExecutionContextImpl *ec = reinterpret_cast<ExecutionContextImpl*>(ecPtr);
    ec->dispatchResult(primitiveType, valuePtr);
}

std::unique_ptr<ExecutionContext> createExecutionContext() {

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    return std::make_unique<ExecutionContextImpl>();
}
} }
