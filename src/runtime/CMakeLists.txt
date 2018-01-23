


set(RUNTIME_SOURCE_FILES ${ANODE_INCLUDE_DIR}/runtime/builtins.h builtins.cpp ../include/anode/ast/AstVisitor.h)

add_library(anode-runtime ${RUNTIME_SOURCE_FILES})


