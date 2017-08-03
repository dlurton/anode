

setup_llvm()

set(EXECUTE_SOURCE_FILES
        execute.cpp
        ${CMAKE_SOURCE_DIR}/src/include/lwnn/string.h)

add_library(lwnn-execute ${EXECUTE_SOURCE_FILES})
target_link_libraries(lwnn-execute lwnn-front lwnn-back ${LLVM_LIBS})
