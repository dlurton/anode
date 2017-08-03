set(CMAKE_CXX_STANDARD 14)

setup_llvm()

set(EXECUTE_SOURCE_FILES
        execute.cpp)

add_library(lwnn-execute ${EXECUTE_SOURCE_FILES})
target_link_libraries(lwnn-execute lwnn-front lwnn-back ${LLVM_LIBS})
