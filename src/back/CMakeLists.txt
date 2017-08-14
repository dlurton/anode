

setup_llvm()

set(COMPILE_SOURCE_FILES
        ${LWNN_INCLUDE_DIR}/common
        ${LWNN_INCLUDE_DIR}/back
        ModuleAstVisitor.cpp
        CompileContext.h
        llvm.h
        CompileAstVisitor.h
        emit.h
        ExprStmtAstVisitor.cpp CreateStructsAstVisitor.h)

add_library(lwnn-back ${COMPILE_SOURCE_FILES})
target_link_libraries(lwnn-back lwnn-front ${LLVM_LIBS})
