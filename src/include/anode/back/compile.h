
#pragma once
#include "anode.h"
#include "front/ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wgnu-statement-expression"
#include <cstring>
#include <llvm/Target/TargetMachine.h>
#include "llvm/IR/Module.h"
#pragma GCC diagnostic pop

//namespace llvm {
//    class Module;
//    class LLVMContext;
//    class TargetMachine;
//}

namespace anode { namespace back {
    const char * const MODULE_INIT_SUFFIX = "__initModule__";
    const char * const RECEIVE_RESULT_FUNC_NAME = "__receive_result__";
    const char * const ASSERT_FAILED_FUNC_NAME = "__assert_failed__";
    const char * const ASSERT_PASSED_FUNC_NAME = "__assert_passed__";
    const char * const MALLOC_FUNC_NAME = "__malloc__";
    const char * const EXECUTION_CONTEXT_GLOBAL_NAME = "__execution__context__";

    class TypeMap : no_assign, no_copy {
        gc_unordered_map<const front::type::Type *, llvm::Type *> typeMap_;
        llvm::LLVMContext &llvmContext_;
    public:
        TypeMap(llvm::LLVMContext &llvmContext) : llvmContext_{llvmContext} {

            mapTypes(&front::type::Primitives::Void, llvm::Type::getVoidTy(llvmContext_));
            mapTypes(&front::type::Primitives::Bool, llvm::Type::getInt1Ty(llvmContext_));
            mapTypes(&front::type::Primitives::Int32, llvm::Type::getInt32Ty(llvmContext_));
            mapTypes(&front::type::Primitives::Float, llvm::Type::getFloatTy(llvmContext_));
            mapTypes(&front::type::Primitives::Double, llvm::Type::getDoubleTy(llvmContext_));
        }

        void mapTypes(front::type::Type *anodeType, llvm::Type *llvmType) {
            typeMap_[anodeType] = llvmType;
        }

        llvm::Type *toLlvmType(front::type::Type *anodeType) {
            ASSERT(anodeType);
            front::type::Type *actualType = anodeType->actualType();
            llvm::Type *foundType = typeMap_[actualType];
            
            auto classType = dynamic_cast<front::type::ClassType*>(actualType);
            if(foundType == nullptr && classType != nullptr) {
                gc_vector<front::type::ClassField*> fields = classType->fields();

                std::vector<llvm::Type *> fieldTypes;
                fieldTypes.reserve(fields.size());

                llvm::StructType *structType = llvm::StructType::create(llvmContext_, classType->nameForDisplay());
                llvm::PointerType *pointerType = structType->getPointerTo(0);
                mapTypes(classType, pointerType);

                for (auto s : fields) {
                    fieldTypes.push_back(toLlvmType(s->type()));
                }

                structType->setBody(fieldTypes);
                return pointerType;
            }

            ASSERT(foundType && "Anode type to LLVM type Mapping must exist!");
            return foundType;
        }
    };

    std::unique_ptr<llvm::Module> emitModule(
        anode::front::ast::AnodeWorld &world,
        anode::front::ast::Module *module,
        anode::back::TypeMap &typeMap,
        llvm::LLVMContext &llvmContext,
        llvm::TargetMachine *targetMachine
    );

}}