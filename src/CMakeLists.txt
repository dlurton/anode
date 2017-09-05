set(CMAKE_CXX_STANDARD 14)

set(LINK_LLVM_DYLIB yes)

set(EXTERNS_DIR "${CMAKE_SOURCE_DIR}/externs")
set(ANODE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/anode")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -Wall -Werror -Wextra -O0 -Wpedantic")

#https://github.com/ivmai/bdwgc/blob/76fd95b4803197954c60c585e4dfc795c4d5c533/doc/README.linux
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-defsym,_DYNAMIC=0")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${ANODE_INCLUDE_DIR})


# NOTE: yes, the "lib/include" and "lib/lib" here is stupid -- libgc also builds libcords which complains if it's not
# being installed in a path that doesn't end with /usr/local/lib.  Of course that means everything is installed in these silly paths when
# I'm not using the defaults.
include_directories(${CMAKE_SOURCE_DIR}/externs/bdwgc/usr/local/lib/include)
find_library(LIB_GC gc PATHS ${EXTERNS_DIR}/bdwgc/usr/local/lib/lib NO_DEFAULT_PATH)
message(STATUS "Found libgc: ${LIB_GC}")

###########################################################################################################
# Setup LLVM.  This function is provided so that components which need to link with LLVM may easily do so.
###########################################################################################################
function(setup_llvm)
    set(LLVM_DIR ${CMAKE_SOURCE_DIR}/externs/llvm/lib/cmake/llvm)
    find_package(LLVM REQUIRED CONFIG)

    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    if(${LINK_LLVM_DYLIB})
        set(LLVM_LIBS LLVM PARENT_SCOPE)
    else()
        llvm_map_components_to_libnames(LLVM_LIBS_TEMP all)
        set(LLVM_LIBS ${LLVM_LIBS_TEMP} PARENT_SCOPE)
    endif()
endfunction()


add_subdirectory(front)
add_subdirectory(back)
add_subdirectory(execute)
add_subdirectory(anode)
add_subdirectory(runtime)
add_subdirectory(tests)



