#include "test_util.h"
#include "front/parse.h"

#include <chrono>

namespace lwnn { namespace test_util {

unsigned int testCount = 0;
unsigned int totalDuration = 0;

std::vector<lwnn::execute::StmtResult> testWithResults(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
#ifdef VISUALIZE_AST
    executionContext->setPrettyPrintAst(true);
#endif

#ifdef DUMP_IR
    executionContext->setDumpIROnLoad(true);
#endif

    std::cerr << "Executing:  " << source << "\n";
    std::string module_name = string::format("test_%d", ++testCount);
    auto testStartTime = std::chrono::high_resolution_clock::now();

    ast::Module *module = parse::parseModule(source, module_name);
    ASSERT(module && "If module is null, a syntax error probably occurred!");

    executionContext->prepareModule(module);

    std::vector<lwnn::execute::StmtResult> results;

    executionContext->setResultCallback(
        [&](execute::ExecutionContext*, type::PrimitiveType primitiveType, void* valuePtr) {
            results.emplace_back(primitiveType, valuePtr);
        });

    executionContext->executeModule(module);
    auto elapsed = std::chrono::high_resolution_clock::now() - testStartTime;
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    std::cerr << milliseconds << "ms\n";
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