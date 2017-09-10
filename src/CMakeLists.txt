set(CMAKE_CXX_STANDARD 14)
project(anode)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DANODE_DEBUG=1)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(LINK_LLVM_DYLIB yes)

set(EXTERNS_DIR "${CMAKE_SOURCE_DIR}/externs/${CMAKE_BUILD_TYPE}")
string(TOLOWER ${EXTERNS_DIR} EXTERNS_DIR)
set(ANODE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/anode")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wpedantic -stdlib=libstdc++")


#https://github.com/ivmai/bdwgc/blob/76fd95b4803197954c60c585e4dfc795c4d5c533/doc/README.linux
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-defsym,_DYNAMIC=0")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

string(TOLOWER ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
string(TOLOWER ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} CMAKE_LIBRARY_OUTPUT_DIRECTORY)
string(TOLOWER ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CMAKE_RUNTIME_OUTPUT_DIRECTORY)

include_directories(${ANODE_INCLUDE_DIR})


# NOTE: yes, the "lib/include" and "lib/lib" here is stupid -- libgc also builds libcords which complains if it's not
# being installed in a path that doesn't end with /usr/local/lib.  Of course that means everything is installed in these silly paths when
# I'm not using the defaults.
include_directories(${EXTERNS_DIR}/bdwgc/usr/local/lib/include)
find_library(LIB_GC gc PATHS ${EXTERNS_DIR}/bdwgc/usr/local/lib/lib NO_DEFAULT_PATH)
message(STATUS "Found libgc: ${LIB_GC}")

###########################################################################################################
# Setup LLVM.  This function is provided so that components which need to link with LLVM may easily do so.
###########################################################################################################
function(setup_llvm)
    set(LLVM_DIR ${EXTERNS_DIR}/llvm/lib/cmake/llvm)
    string(TOLOWER ${LLVM_DIR} LLVM_DIR)
        message("LLVM_DIR: ${LLVM_DIR}")

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



