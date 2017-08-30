

setup_llvm()

#linenoise
include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime ${EXTERNS_DIR}/linenoise-ng/include)
find_library(LIB_LINENOISE linenoise PATHS ${EXTERNS_DIR}/linenoise-ng/lib)

set(LWNN_SOURCE_FILES main.cpp ../include/lwnn/common/containers.h ../front/LwnnParser.h ../include/lwnn/front/parse.h)

add_executable(lwnn ${LWNN_SOURCE_FILES})

target_link_libraries(lwnn lwnn-back lwnn-front lwnn-execute ${LIB_LINENOISE} ${LLVM_LIBS} ${LIB_GC})

