set(CMAKE_CXX_STANDARD 14)

setup_llvm()

set(EXECUTE_SOURCE_FILES
        ${LWNN_INCLUDE_DIR}/common
        ${LWNN_INCLUDE_DIR}/execute
        ${LWNN_INCLUDE_DIR}/back
        ${LWNN_INCLUDE_DIR}/front
        ExecutionContextImpl.cpp
        LwnnJit.h
        llvm.h)

add_library(lwnn-execute ${EXECUTE_SOURCE_FILES})
target_link_libraries(lwnn-execute lwnn-front lwnn-back ${LLVM_LIBS})
