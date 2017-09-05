set(CMAKE_CXX_STANDARD 14)

setup_llvm()

set(EXECUTE_SOURCE_FILES
        ${ANODE_INCLUDE_DIR}/common
        ${ANODE_INCLUDE_DIR}/execute
        ${ANODE_INCLUDE_DIR}/back
        ${ANODE_INCLUDE_DIR}/front
        ExecutionContextImpl.cpp
        AnodeJit.h
        llvm.h)

add_library(anode-execute ${EXECUTE_SOURCE_FILES})
target_link_libraries(anode-execute anode-front anode-back anode-runtime ${LLVM_LIBS})
