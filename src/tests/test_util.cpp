#include "test_util.h"

#include <chrono>
#include "catch.hpp"
#include <front/parse.h>

namespace anode { namespace test_util {

unsigned int testCount = 0;
unsigned int totalDuration = 0;

std::vector<anode::execute::StmtResult> testWithResults(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    //The idea of this is to force garbage collection to occur as early as possible so that premature collection bugs are exposed sooner.
    //while(GC_collect_a_little());

#ifdef VISUALIZE_AST
    executionContext->setPrettyPrintAst(true);
#endif

#ifdef DUMP_IR
    executionContext->setDumpIROnLoad(true);
#endif

    std::cerr << "Executing:  " << source << "\n";
    std::string module_name = string::format("test_%d", ++testCount);
    auto testStartTime = std::chrono::high_resolution_clock::now();

    front::ast::Module *module = nullptr;
    try {
        module = front::parseModule(source, module_name);
    } catch(anode::front::ParseAbortedException&) {
        FAIL("Parse aborted.");
    }

    if(executionContext->prepareModule(module)) {
        FAIL("Module preparation failed.");
    }

    std::vector<anode::execute::StmtResult> results;

    executionContext->setResultCallback(
        [&](execute::ExecutionContext*, front::type::PrimitiveType primitiveType, void* valuePtr) {
            results.emplace_back(primitiveType, valuePtr);
        });

    executionContext->executeModule(module);
    auto elapsed = std::chrono::high_resolution_clock::now() - testStartTime;
    long long milliseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cerr << std::fixed << milliseconds << "us\n";
    totalDuration += milliseconds;
    //GC_collect_a_little();
    return results;
}

void exec(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    testWithResults(executionContext, source);
}

void exec(std::string source) {
    std::shared_ptr<execute::ExecutionContext> executionContext = execute::createExecutionContext();
    testWithResults(executionContext, source);
}

}}