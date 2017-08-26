setup_llvm()

set(TESTS_SOURCE_FILES simple_tests.cpp ../../externs/catch/catch.hpp test_util.h test_util.cpp)

include_directories(${CMAKE_SOURCE_DIR}/externs/catch)

add_executable(simple_tests ${TESTS_SOURCE_FILES} )

target_link_libraries(simple_tests lwnn-back lwnn-front lwnn-execute ${LLVM_LIBS} ${LIB_GC})


add_test(NAME simple_tests COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_tests)

file(GLOB test_suites "suites/*.lwnn")
foreach(file ${test_suites})
    get_filename_component(test_name ${file} NAME_WE)
    add_test(
        NAME test-${test_name}
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lwnn ${file})

endforeach()