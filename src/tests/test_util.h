//#define VISUALIZE_AST
//#define DUMP_IR

#pragma once
#include "front/type.h"
#include "execute/execute.h"

#include <execute/execute.h>

namespace anode { namespace test_util {

extern unsigned int testCount;
extern unsigned int totalDuration;

void exec(std::string source);
void exec(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source);

std::vector<anode::execute::StmtResult> testWithResults(std::shared_ptr<anode::execute::ExecutionContext> executionContext, std::string source);

/** This variant expects there to be only one result */
template<typename T>
T test(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    std::vector<anode::execute::StmtResult> results = testWithResults(executionContext, source);
    ASSERT(results.size() == 1);
    return results[0].get<T>();
}

template<typename T>
T test(std::string source) {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    T result = test<T>(ec, source);
    return result;
}

}}