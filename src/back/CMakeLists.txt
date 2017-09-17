

setup_llvm()

set(COMPILE_SOURCE_FILES
        ${ANODE_INCLUDE_DIR}/common
        ${ANODE_INCLUDE_DIR}/back
        ModuleEmitter.cpp
        CompileContext.h
        llvm.h
        CompileAstVisitor.h
        emit.h
        ExprStmtAstVisitor.cpp CreateStructsAstVisitor.h FuncDefStmtVistors.cpp GlobalVariableAstVisitor.h)

add_library(anode-back ${COMPILE_SOURCE_FILES})
target_link_libraries(anode-back anode-front ${LLVM_LIBS})
