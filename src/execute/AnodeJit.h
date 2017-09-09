
#pragma once

#include "llvm.h"

namespace anode {
    namespace execute {

        std::unique_ptr<llvm::Module> irgenAndTakeOwnership(ast::FuncDefStmt &FnAST, const std::string &Suffix);

        //The original version of this (llvm::orc::createResolver(...)) doesn't seem to want to compile no matter what I do.
        template<typename DylibLookupFtorT, typename ExternalLookupFtorT>
        std::unique_ptr<llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT>>
        createLambdaResolver2(DylibLookupFtorT DylibLookupFtor, ExternalLookupFtorT ExternalLookupFtor) {
            typedef llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT> LR;
            return std::make_unique<LR>(DylibLookupFtor, ExternalLookupFtor);
        }

        /** This class originally taken from:
         * https://github.com/llvm-mirror/llvm/tree/master/examples/Kaleidoscope/BuildingAJIT/Chapter4
         * http://llvm.org/docs/tutorial/BuildingAJIT4.html
         */
        class AnodeJit {
        private:
            std::unique_ptr<llvm::TargetMachine> TM;
            const llvm::DataLayout DL;
            llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
            llvm::orc::IRCompileLayer<decltype(ObjectLayer), llvm::orc::SimpleCompiler> CompileLayer;

            using OptimizeFunction = std::function<std::shared_ptr<llvm::Module>(std::shared_ptr<llvm::Module>)>;

            llvm::orc::IRTransformLayer<decltype(CompileLayer), OptimizeFunction> OptimizeLayer;

            std::unique_ptr<llvm::orc::JITCompileCallbackManager> CompileCallbackMgr;
            std::unique_ptr<llvm::orc::IndirectStubsManager> IndirectStubsMgr;

            std::unordered_map<std::string, runtime::symbolptr_t> exports_;
            bool enableOptimization_ = true;
        public:
            using ModuleHandle = decltype(OptimizeLayer)::ModuleHandleT;

            AnodeJit()
                : TM(llvm::EngineBuilder().selectTarget()),
                  DL(TM->createDataLayout()),
                  ObjectLayer([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
                  CompileLayer(ObjectLayer, llvm::orc::SimpleCompiler(*TM)),
                  OptimizeLayer(CompileLayer,
                                [this](std::shared_ptr<llvm::Module> M) {
                                    return optimizeModule(std::move(M));
                                }),
                  CompileCallbackMgr(
                      llvm::orc::createLocalCompileCallbackManager(TM->getTargetTriple(), 0))
            {
                auto IndirectStubsMgrBuilder = llvm::orc::createLocalIndirectStubsManagerBuilder(TM->getTargetTriple());
                IndirectStubsMgr = IndirectStubsMgrBuilder();
                llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
            }

            /** Adds a symbol to be exported to the JIT'd modules, overwriting any previously added values.*/
            void putExport(std::string name, runtime::symbolptr_t address) {
                auto found = exports_.find(name);
                if(found != exports_.end()) {
                    exports_.erase(found);
                }
                exports_.emplace(name, address);
            }

            llvm::TargetMachine *getTargetMachine() { return TM.get(); }

            ModuleHandle addModule(std::shared_ptr<llvm::Module> M) {
                // Build our symbol resolver:
                // Lambda 1: Look back into the JIT itself to find symbols that are part of
                //           the same "logical dylib".
                // Lambda 2: Search for external symbols in the host process.
                auto Resolver = createLambdaResolver2(
                    [&](const std::string &Name) {
                        if (auto Sym = IndirectStubsMgr->findStub(Name, false))
                            return Sym;
                        if (auto Sym = OptimizeLayer.findSymbol(Name, false))
                            return Sym;
                        return llvm::JITSymbol(nullptr);
                    },
                    [&](const std::string &Name) {
                        auto foundExport = exports_.find(Name);
                        if(foundExport != exports_.end()) {
                            return llvm::JITSymbol(llvm::JITTargetAddress(foundExport->second), llvm::JITSymbolFlags::Exported);
                        }

                        if (auto SymAddr =
                            llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
                            return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
                        return llvm::JITSymbol(nullptr);
                    });

                // Add the set to the JIT with the resolver we created above and a newly
                // created SectionMemoryManager.
                return cantFail(OptimizeLayer.addModule(std::move(M),
                                                        std::move(Resolver)));
            }

//            llvm::Error addFunctionAST(std::unique_ptr<ast::FuncDefStmt> FnAST) {
//                // Create a CompileCallback - this is the re-entry point into the compiler
//                // for functions that haven't been compiled yet.
//                auto CCInfo = CompileCallbackMgr->getCompileCallback();
//
//                // Create an indirect stub. This serves as the functions "canonical
//                // definition" - an unchanging (constant address) entry point to the
//                // function implementation.
//                // Initially we point the stub's function-pointer at the compile callback
//                // that we just created. In the compile action for the callback (see below)
//                // we will update the stub's function pointer to point at the function
//                // implementation that we just implemented.
//                if (auto Err = IndirectStubsMgr->createStub(mangle(FnAST->name()),
//                                                            CCInfo.getAddress(),
//                                                            llvm::JITSymbolFlags::Exported))
//                    return Err;
//
//                // Move ownership of FnAST to a shared pointer - C++11 lambdas don't support
//                // capture-by-move, which is be required for unique_ptr.
//                auto SharedFnAST = std::shared_ptr<ast::FuncDefStmt>(std::move(FnAST));
//
//                // Set the action to compile our AST. This lambda will be run if/when
//                // execution hits the compile callback (via the stub).
//                //
//                // The steps to compile are:
//                // (1) IRGen the function.
//                // (2) Add the IR module to the JIT to make it executable like any other
//                //     module.
//                // (3) Use findSymbol to get the address of the compiled function.
//                // (4) Update the stub pointer to point at the implementation so that
//                ///    subsequent calls go directly to it and bypass the compiler.
//                // (5) Return the address of the implementation: this lambda will actually
//                //     be run inside an attempted call to the function, and we need to
//                //     continue on to the implementation to complete the attempted call.
//                //     The JIT runtime (the resolver block) will use the return address of
//                //     this function as the address to continue at once it has reset the
//                //     CPU state to what it was immediately before the call.
//                CCInfo.setCompileAction(
//                    [this, SharedFnAST]() {
//                        auto M = irgenAndTakeOwnership(*SharedFnAST, "$impl");
//                        addModule(std::move(M));
//                        auto Sym = findSymbol(SharedFnAST->name() + "$impl");
//                        assert(Sym && "Couldn't find compiled function?");
//                        llvm::JITTargetAddress SymAddr = cantFail(Sym.getAddress());
//                        if (auto Err =
//                            IndirectStubsMgr->updatePointer(mangle(SharedFnAST->name()),
//                                                            SymAddr)) {
//                            logAllUnhandledErrors(std::move(Err), llvm::errs(),
//                                                  "Error updating function pointer: ");
//                            exit(1);
//                        }
//
//                        return SymAddr;
//                    });
//
//                return llvm::Error::success();
//            }

            llvm::JITSymbol findSymbol(const std::string Name) {
                return OptimizeLayer.findSymbol(mangle(Name), true);
            }

            void removeModule(ModuleHandle H) {
                cantFail(OptimizeLayer.removeModule(H));
            }

            void setEnableOptimization(bool enableOptimization) {
                enableOptimization_ = enableOptimization;
            }

        private:
            std::string mangle(const std::string &Name) {
                std::string MangledName;
                llvm::raw_string_ostream MangledNameStream(MangledName);
                llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
                return MangledNameStream.str();
            }

            std::shared_ptr<llvm::Module> optimizeModule(std::shared_ptr<llvm::Module> M) {

                if(!enableOptimization_) return M;

                // Create a function pass manager.
                auto FPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(M.get());

                // Add some optimizations.
                FPM->add(llvm::createInstructionCombiningPass());
                FPM->add(llvm::createReassociatePass());
                FPM->add(llvm::createGVNPass());
                FPM->add(llvm::createCFGSimplificationPass());
                FPM->doInitialization();

                // Run the optimizations over all functions in the module being added to
                // the JIT.
                for (auto &F : *M)
                    FPM->run(F);
                return M;
            }
        }; //SimpleJIT

    }
}