//#define VISUALIZE_AST
//#define DUMP_IR


#pragma once
#include "front/type.h"
#include "execute/execute.h"

#include <execute/execute.h>

namespace lwnn {
    namespace test_util {

    union ResultStorage {
            bool boolResult;
            int int32Result;
            float floatReslt;
            double doubleResult;
        };

        struct StmtResult {
            type::PrimitiveType primitiveType;
            ResultStorage storage;

            template<typename T>
            T get() {
                if (typeid(T) == typeid(bool)) {
                    ASSERT(primitiveType == type::PrimitiveType::Bool);
                    return storage.boolResult;
                }
                if(typeid(T) == typeid(int)) {
                    ASSERT(primitiveType == type::PrimitiveType::Int32);
                    return storage.int32Result;
                }
                if(typeid(T) == typeid(float)) {
                    ASSERT(primitiveType == type::PrimitiveType::Float);
                    return storage.floatReslt;
                }
                if(typeid(T) == typeid(double)) {
                    ASSERT(primitiveType == type::PrimitiveType::Double);
                    return storage.doubleResult;
                }

                ASSERT_FAIL("T may be only bool, int, float or double");
            }
        };

        void exec(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source);
        std::vector<StmtResult> testWithResults(std::shared_ptr<lwnn::execute::ExecutionContext> executionContext, std::string source);

        /** This variant expects there to be only one result */
        template<typename T>
        T test(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
            std::vector<StmtResult> results = testWithResults(executionContext, source);
            ASSERT(results.size() == 1);
            return results[0].get<T>();
        }

        template<typename T>
        T test(std::string source) {
            std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
            T result = test<T>(ec, source);
            return result;
        }

    }
}