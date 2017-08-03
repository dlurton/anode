setup_llvm()

set(TESTS_SOURCE_FILES tests.cpp catch.hpp)

add_executable(tests ${TESTS_SOURCE_FILES})

target_link_libraries(tests lwnn-back lwnn-front lwnn-execute ${LLVM_LIBS})

enable_testing()

add_test(tests tests)