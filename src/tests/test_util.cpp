#include "test_util.h"
#include "front/parse.h"

#include <chrono>

namespace lwnn { namespace test_util {
int testCount = 0;

std::vector<StmtResult> testWithResults(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
#ifdef VISUALIZE_AST
    executionContext->setPrettyPrintAst(true);
#endif

#ifdef DUMP_IR
    executionContext->setDumpIROnLoad(true);
#endif

    std::cout << "Executing:  " << source << "\n";
    std::string module_name = string::format("test_%d", ++testCount);
    auto testStartTime = std::chrono::high_resolution_clock::now();

    ast::Module *module = parse::parseModule(source, module_name);
    ASSERT(module && "If module is null, a syntax error probably occurred!");

    executionContext->prepareModule(module);

    std::vector<StmtResult> results;

    executionContext->setResultCallback(
        [&](execute::ExecutionContext*, type::PrimitiveType primitiveType, uint64_t valuePtr) {
            StmtResult result;
            result.primitiveType = primitiveType;
            switch (primitiveType) {
                case type::PrimitiveType::Bool:
                    result.storage.boolResult = *reinterpret_cast<bool*>(valuePtr);
                    break;
                case type::PrimitiveType::Int32:
                    result.storage.int32Result = *reinterpret_cast<int*>(valuePtr);
                    break;
                case type::PrimitiveType::Float:
                    result.storage.floatReslt = *reinterpret_cast<float*>(valuePtr);
                    break;
                case type::PrimitiveType::Double:
                    result.storage.doubleResult = *reinterpret_cast<double*>(valuePtr);
                    break;
                default:
                    ASSERT_FAIL("Unhandled PrimitiveType");
            }
            results.push_back(result);
        });

    executionContext->executeModule(module);
    auto elapsed = std::chrono::high_resolution_clock::now() - testStartTime;
    long long microseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    std::cout << microseconds << "ms\n";

    //GC_collect_a_little();
    return results;
}

void exec(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    testWithResults(executionContext, source);
}

}}