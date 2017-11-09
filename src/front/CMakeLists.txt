##############################################################
# Build anode
##############################################################
set(FRONT_SRC_FILES
        ${ANODE_INCLUDE_DIR}/common
        ${ANODE_INCLUDE_DIR}/front
        ${ANODE_INCLUDE_DIR}/front/type.h
        ${ANODE_INCLUDE_DIR}/front/ErrorStream.h
        ast.cpp
        visualize.cpp
        type.cpp
        passes/ast_passes.cpp
        source.cpp
        passes/AddImplicitCastsPass.h
        parser/AnodeLexer.cpp
        parser/AnodeLexer.h
        parser/PrattParser.h
        parser/Token.h
        parser/char.h
        parser/AnodeParser.cpp
        SourceReader.h
        parse.cpp scope.cpp unique_id.cpp ../include/anode/front/unique_id.h ../include/anode/common/enum.h)

find_library(LIB_UUID uuid)

add_library(anode-front ${FRONT_SRC_FILES})

target_link_libraries(anode-front ${LIB_UUID})