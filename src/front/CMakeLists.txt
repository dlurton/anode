cmake_minimum_required(VERSION 3.5)
project(lwnn-front CXX)

##############################################################
# Build lwnn
##############################################################
set(FRONT_SRC_FILES
        ${LWNN_INCLUDE_DIR}/common
        ${LWNN_INCLUDE_DIR}/front
        ast.cpp
        visualize.cpp
        type.cpp
        passes/ast_passes.cpp
        source.cpp
        passes/AddImplicitCastsPass.h
        Lexer.cpp
        Lexer.h
        PrattParser.h
        Token.h
        char.h
        SourceReader.h
        LwnnParser.cpp
        parse.cpp)

include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime ${EXTERNS_DIR}/linenoise-ng/include)

find_library(LIB_ANTLR4_RUNTIME antlr4-runtime PATHS ${EXTERNS_DIR}/antlr4/usr/local/lib)

add_library(lwnn-front ${FRONT_SRC_FILES})

target_link_libraries(lwnn-front ${LIB_ANTLR4_RUNTIME} )
