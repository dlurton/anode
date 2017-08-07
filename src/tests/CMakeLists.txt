setup_llvm()

set(TESTS_SOURCE_FILES integration_tests.cpp ../../externs/catch/catch.hpp test_util.h test_util.cpp)

include_directories(${CMAKE_SOURCE_DIR}/externs/catch)

add_executable(tests ${TESTS_SOURCE_FILES} )

target_link_libraries(tests lwnn-back lwnn-front lwnn-execute ${LLVM_LIBS} ${LIB_GC})

enable_testing()

add_test(tests tests)