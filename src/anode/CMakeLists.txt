

setup_llvm()

#linenoise
include_directories(${EXTERNS_DIR}/linenoise-ng/include)
find_library(LIB_LINENOISE linenoise PATHS ${EXTERNS_DIR}/linenoise-ng/lib)

set(ANODE_SOURCE_FILES main.cpp ../include/anode/common/containers.h ../front/parser/AnodeParser.h ../include/anode/front/parse.h)

find_library(LIB_UUID uuid REQUIRED)
add_executable(anode ${ANODE_SOURCE_FILES})

target_link_libraries(anode anode-back anode-front anode-execute ${LIB_LINENOISE} ${LLVM_LIBS} ${LIB_GC} ${LIB_UUID})

