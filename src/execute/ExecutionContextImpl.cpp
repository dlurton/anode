#include "execute/execute.h"
#include "back/compile.h"
#include "front/ast_passes.h"
#include "front/visualize.h"
#include "runtime/builtins.h"

#include "AnodeJit.h"
namespace anode { namespace execute {

using namespace anode::front;

/** This function is called by the JITd code to deliver the result of an expression entered at the REPL. */
extern "C" void receiveReplResult(uint64_t ecPtr, type::PrimitiveType primitiveType, void *valuePtr);


AnodeJit *Jit;

void InitializeJit() {
    if(!Jit) {
        Jit = new AnodeJit();
        Jit->setEnableOptimization(false);
        Jit->putExport(back::RECEIVE_RESULT_FUNC_NAME, reinterpret_cast<runtime::symbolptr_t>(receiveReplResult));

        auto builtins = anode::runtime::getBuiltins();
        for (auto &pair : builtins) {
            Jit->putExport(pair.first, pair.second);
        }
    }
};


/** This class contains contain pointers to garbage collected objects so it *must* inherit from gc or gc_cleanup
 * so that it's memory is scanned for  pointers to live objects, otherwise these may get collected prematurely.
 * Most likely, there can only ever be one instance of this at a time--but that would depend on if AnodeJit is
 * thread-safe and that depends mostly on LLVM. */
class ExecutionContextImpl : public ExecutionContext {
    llvm::LLVMContext context_;
    bool dumpIROnModuleLoad_ = false;
    bool setPrettyPrintAst_ = false;
    ast::AnodeWorld world_;
    ResultCallbackFunctor resultFunctor_ = nullptr;
    back::TypeMap typeMap_;
public:
    NO_COPY_NO_ASSIGN(ExecutionContextImpl)
    ExecutionContextImpl() : typeMap_{context_} {
        Jit->putExport(back::EXECUTION_CONTEXT_GLOBAL_NAME, reinterpret_cast<runtime::symbolptr_t>(this));
    }

    void dispatchResult(type::PrimitiveType primitiveType, void *valuePtr) {
        if(resultFunctor_)
            resultFunctor_(this, primitiveType, valuePtr);
    }

    uint64_t getSymbolAddress(const std::string &name) override {
        llvm::JITSymbol symbol = Jit->findSymbol(name);

        if (!symbol)
            return 0;

        uint64_t retval = llvm::cantFail(symbol.getAddress());
        return retval;
    }

    void setDumpIROnLoad(bool value) override {
        dumpIROnModuleLoad_ = value;
    }

    void setPrettyPrintAst(bool value) override {
        setPrettyPrintAst_ = value;
    }

    void setResultCallback(ResultCallbackFunctor functor) override {
        resultFunctor_ = functor;
    }

    bool prepareModule(ast::Module *module) override {
        error::ErrorStream errorStream {std::cerr};
        anode::front::passes::runAllPasses(world_, *module, errorStream);

        if(errorStream.errorCount() > 0) {
            return true;
        }

        if(setPrettyPrintAst_) {
            visualize::prettyPrint(*module);
            std::cout.flush();
        }

        return false;
    }

protected:
    virtual uint64_t loadModule(ast::Module *module) override {
        ASSERT(module);

        std::unique_ptr<llvm::Module> llvmModule = back::emitModule(world_, module, typeMap_, context_, Jit->getTargetMachine());

        if(dumpIROnModuleLoad_) {
#ifdef ANODE_DEBUG
            std::cerr << "LLVM IR:\n";
            llvmModule->dump();
#endif
        }

        Jit->addModule(move(llvmModule));

        if(auto moduleInitSymbol = Jit->findSymbol(module->name() + back::MODULE_INIT_SUFFIX))
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

    InitializeJit();

    return std::make_unique<ExecutionContextImpl>();
}
} }
