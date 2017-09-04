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
        parser/Lexer.cpp
        parser/Lexer.h
        parser/PrattParser.h
        parser/Token.h
        parser/char.h
        SourceReader.h
        parser/LwnnParser.cpp
        parse.cpp scope.cpp)

find_library(LIB_UUID uuid)

add_library(lwnn-front ${FRONT_SRC_FILES})

target_link_libraries(lwnn-front ${LIB_UUID})