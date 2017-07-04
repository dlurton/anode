set(CMAKE_CXX_STANDARD 14)

set(EXTERNS_DIR "${CMAKE_SOURCE_DIR}/externs")
set(LINK_LLVM_DYLIB yes)

##############################################################
# ANTLR4 Parser Generation
##############################################################
set(ANTLR_JAR_LOCATION ${EXTERNS_DIR}/antlr4/antlr4-complete.jar)

set(GENERATED_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnBaseListener.h
		${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnBaseListener.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnBaseVisitor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnVisitor.h
		${CMAKE_CURRENT_SOURCE_DIR}/generated/LwnnVisitor.cpp
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
		COMMAND	"java" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -listener -visitor -o ${CMAKE_CURRENT_SOURCE_DIR}/generated/ -package LwnnGeneratedParser ${CMAKE_CURRENT_SOURCE_DIR}/Lwnn.g4
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


##############################################################
# Build lwnn
##############################################################
set(SOURCE_FILES ${GENERATED_SOURCE_FILES}
		main.cpp
		antlr4_demo.cpp
        llvm_demo.cpp
        llvm_demo.cpp)

include_directories(${EXTERNS_DIR}/antlr4/usr/local/include/antlr4-runtime ${EXTERNS_DIR}/linenoise-ng/include)

find_library(LIB_ANTLR4_RUNTIME antlr4-runtime PATHS ${EXTERNS_DIR}/antlr4/usr/local/lib)
find_library(LIB_LINENOISE linenoise PATHS ${EXTERNS_DIR}/linenoise-ng/lib)

add_executable(lwnn ${SOURCE_FILES})
target_link_libraries(lwnn ${LIB_LINENOISE} ${LIB_ANTLR4_RUNTIME} ${LLVM_LIBS})


