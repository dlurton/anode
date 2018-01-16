
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
        parse.cpp
        scope.cpp
        unique_id.cpp
        ${ANODE_INCLUDE_DIR}/front/unique_id.h
        ${ANODE_INCLUDE_DIR}/common/enum.h
        passes/symbol_search.cpp
        passes/symbol_search.h
        passes/PopulateSymbolTablesPass.h
        passes/ScopeFollowingAstVisitor.h
        passes/ErrorContextAstVisitor.h
        passes/SetSymbolTableParentsPass.h
        passes/ResolveSymbolsPass.h
        passes/ResolveTypesPass.h
        passes/CastExprSemanticPass.h
        passes/ResolveDotExprMemberPass.h
        passes/BinaryExprSemanticsPass.h
        passes/FuncCallSemanticsPass.h
        passes/NamedTemplateExpanderPass.h
        passes/run_passes.h
        passes/PopulateGenericTypesWithCompleteTypesPass.h
        passes/ConvertGenericTypeRefsToCompletePass.h
        passes/AnonymousTemplateSemanticPass.h
        passes/ExpandGenericTypeReferencesPass.h)



add_library(anode-front ${FRONT_SRC_FILES})
