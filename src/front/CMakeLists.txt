cmake_minimum_required(VERSION 3.5)
project(lwnn-front CXX)

##############################################################
# ANTLR4 Parser Generation
##############################################################
set(ANTLR_JAR_LOCATION ${EXTERNS_DIR}/antlr4/antlr4-complete.jar)

set(GENERATED_FRONT_SRC_FILES
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnBaseListener.h
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnBaseListener.cpp
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnLexer.h
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnLexer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnListener.h
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnListener.cpp
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnParser.h
        ${CMAKE_CURRENT_LIST_DIR}/generated/LwnnParser.cpp
)

foreach( src_file ${GENERATED_FRONT_SRC_FILES} )
    set_source_files_properties(${src_file} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "-w")
endforeach(src_file ${GENERATED_FRONT_SRC_FILES})

add_custom_command(
        COMMAND "java" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -listener -o generated/ -package lwnn_parser Lwnn.g4
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS Lwnn.g4
        OUTPUT ${GENERATED_FRONT_SRC_FILES}
)


##############################################################
# Build lwnn
##############################################################
set(FRONT_SRC_FILES
        ${GENERATED_FRONT_SRC_FILES}
        ${LWNN_INCLUDE_DIR}/common
        ${LWNN_INCLUDE_DIR}/front
        ast.cpp
        parse.cpp
        visualize.cpp
        type.cpp
        passes/ast_passes.cpp
        source.cpp
        passes/AddImplicitCastsVisitor.h)

include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime ${EXTERNS_DIR}/linenoise-ng/include)

find_library(LIB_ANTLR4_RUNTIME antlr4-runtime PATHS ${EXTERNS_DIR}/antlr4/usr/local/lib)

add_library(lwnn-front ${FRONT_SRC_FILES})

target_link_libraries(lwnn-front ${LIB_ANTLR4_RUNTIME} )
