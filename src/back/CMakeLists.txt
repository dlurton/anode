

setup_llvm()

set(COMPILE_SOURCE_FILES
        ${ANODE_INCLUDE_DIR}/common
        ${ANODE_INCLUDE_DIR}/back
        ${ANODE_INCLUDE_DIR}/anode.h
        ModuleEmitter.cpp
        CompileContext.h
        llvm.h
        CompileAstVisitor.h
        emit.h
        ExprStmtAstVisitor.cpp FuncDefStmtVistors.cpp GlobalVariableAstVisitor.h)

add_library(anode-back ${COMPILE_SOURCE_FILES})
target_link_libraries(anode-back anode-front ${LLVM_LIBS})
