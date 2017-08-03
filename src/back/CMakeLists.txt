

setup_llvm()

set(COMPILE_SOURCE_FILES compile.cpp
        ${CMAKE_SOURCE_DIR}/src/include/lwnn/string.h)

add_library(lwnn-back ${COMPILE_SOURCE_FILES})
target_link_libraries(lwnn-back lwnn-front ${LLVM_LIBS})
