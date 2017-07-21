set(CMAKE_CXX_STANDARD 14)

set(EXTERNS_DIR "${CMAKE_SOURCE_DIR}/externs")
set(LINK_LLVM_DYLIB yes)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

##############################################################
# ANTLR4 Parser Generation
##############################################################
set(ANTLR_JAR_LOCATION ${EXTERNS_DIR}/antlr4/antlr4-complete.jar)

set(GENERATED_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnBaseListener.h
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnBaseListener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnLexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnLexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnListener.h
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnListener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnParser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnParser.cpp
)

foreach( src_file ${GENERATED_SOURCE_FILES} )
    set_source_files_properties(${src_file} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS -Wno-overloaded-virtual)
    set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS -Wno-attributes)

endforeach(src_file ${GENERATED_SOURCE_FILES})

add_custom_command(
        COMMAND    "java" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -listener -o ${CMAKE_CURRENT_SOURCE_DIR}/generated/ -package lwnn_parser ${CMAKE_CURRENT_SOURCE_DIR}/Lwnn.g4
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Lwnn.g4
        OUTPUT ${GENERATED_SOURCE_FILES}
)

##############################################################
# Include LLVM
##############################################################
set(LLVM_DIR ../externs/llvm/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if(${LINK_LLVM_DYLIB})
    set(LLVM_LIBS LLVM)
else()
    llvm_map_components_to_libnames(LLVM_LIBS all)
endif()

#add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

##############################################################
# Build lwnn
##############################################################
set(SOURCE_FILES ${GENERATED_SOURCE_FILES}
        ast.h
        ast.cpp
        exception.h
        parse.cpp
        visualize.h
        visualize.cpp
        execute.h
        execute.cpp
        parse.h
        compile.cpp
        compile.h type.cpp type.h scope.h ast_passes.cpp ast_passes.h error.cpp error.h source.h source.cpp string.h backtrace.cpp backtrace.h)

include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime ${EXTERNS_DIR}/linenoise-ng/include)

find_library(LIB_ANTLR4_RUNTIME antlr4-runtime PATHS ${EXTERNS_DIR}/antlr4/usr/local/lib)
find_library(LIB_LINENOISE linenoise PATHS ${EXTERNS_DIR}/linenoise-ng/lib)

add_executable(lwnn ${SOURCE_FILES} main.cpp)
target_link_libraries(lwnn ${LIB_LINENOISE} ${LIB_ANTLR4_RUNTIME} ${LLVM_LIBS})

add_executable(tests ${SOURCE_FILES} tests.cpp catch.hpp)
target_link_libraries(tests ${LIB_ANTLR4_RUNTIME} ${LLVM_LIBS})

add_test(tests tests)
