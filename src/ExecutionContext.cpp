#include "ExecutionContext.h"
#include "CodeGenVisitor.h"

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

        llvm::TargetMachine *getTargetMachine() { return TM.get(); }

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
            return cantFail(CompileLayer.addModule(M, std::move(Resolver)));
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

    class ExecutionContextImpl : public ExecutionContext {
        //TODO:  determine if definition order (destruction order) is still significant, and if so
        //update this note to say so.
        llvm::LLVMContext context_;
        std::unique_ptr<SimpleJIT> jit_ = std::make_unique<SimpleJIT>();

    public:

        uint64_t getSymbolAddress(const std::string &name) override {
            llvm::JITSymbol symbol = jit_->findSymbol(name);

            if(!symbol)
                return 0;

            uint64_t retval = cantFail(symbol.getAddress());
            return retval;
        }

        void addModule(const Module *module) override  {
            std::unique_ptr<llvm::Module> llvmModule = lwnn::compile::generateCode(module, &context_, jit_->getTargetMachine());
            jit_->addModule(move(llvmModule));
        }
    };

    std::unique_ptr<ExecutionContext> createExecutionContext() {
        return std::make_unique<ExecutionContextImpl>();
    }

    void initializeJitCompiler() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
    }
} //namespace float
