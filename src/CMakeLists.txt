cmake_minimum_required(VERSION 3.6)
project(lwnn)

set(CMAKE_CXX_STANDARD 14)

set(EXTERNS_DIR "${CMAKE_SOURCE_DIR}/../externs")

set(ANTLR_JAR_LOCATION ${EXTERNS_DIR}/antlr4/antlr4-complete.jar)

set(GENERATED_SOURCE_FILES
		${CMAKE_SOURCE_DIR}/generated/HelloBaseListener.h
		${CMAKE_SOURCE_DIR}/generated/HelloBaseListener.cpp
		${CMAKE_SOURCE_DIR}/generated/HelloBaseVisitor.cpp
		${CMAKE_SOURCE_DIR}/generated/HelloVisitor.h
		${CMAKE_SOURCE_DIR}/generated/HelloVisitor.cpp
		${CMAKE_SOURCE_DIR}/generated/HelloLexer.h
		${CMAKE_SOURCE_DIR}/generated/HelloLexer.cpp
		${CMAKE_SOURCE_DIR}/generated/HelloListener.h
		${CMAKE_SOURCE_DIR}/generated/HelloListener.cpp
		${CMAKE_SOURCE_DIR}/generated/HelloParser.h
		${CMAKE_SOURCE_DIR}/generated/HelloParser.cpp
)

foreach( src_file ${GENERATED_SOURCE_FILES} )
	set_source_files_properties(${src_file} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS -Wno-overloaded-virtual)
	set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS -Wno-attributes)

endforeach(src_file ${GENERATED_SOURCE_FILES})
add_custom_command(
		COMMAND	"java" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -listener -visitor -o ${CMAKE_SOURCE_DIR}/generated/ -package LwnnParser ${CMAKE_SOURCE_DIR}/Hello.g4
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS ${CMAKE_SOURCE_DIR}/Hello.g4
		OUTPUT ${GENERATED_SOURCE_FILES}
)

set(SOURCE_FILES main.cpp ${GENERATED_SOURCE_FILES})

link_directories(${EXTERNS_DIR}/antlr4/usr/local/lib)
include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime)

find_library($LIB_ANTLR4_RUNTIME antlr4-runtime)

add_executable(lwnn ${lwnn-generated-src} ${SOURCE_FILES})
target_link_libraries(lwnn antlr4-runtime)


