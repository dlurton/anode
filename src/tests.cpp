
//#define VISUALIZE_AST
//#define DUMP_IR

#include "execute.h"
#include "parse.h"
#include "compile.h"


#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace lwnn;

int testCount = 0;

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


std::vector<StmtResult> testWithResults(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    std::cout << "Executing:  " << source << "\n";
    std::string module_name = string::format("test_%d", ++testCount);

    std::unique_ptr<ast::Module> module = parse::parseModule(source, module_name);
    ASSERT(module && "If module is null, a syntax error probably occurred!");
    executionContext->prepareModule(module.get());

#ifdef VISUALIZE_AST
    executionContext->setPrettyPrintAst(true);
#endif

#ifdef DUMP_IR
    executionContext->setDumpIROnLoad(true);
#endif

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

    executionContext->executeModule(std::move(module));

    return results;
}

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

TEST_CASE("basic bool expressions") {
    SECTION("literals") {
        REQUIRE(test<bool>("true;"));
        REQUIRE(!test<bool>("false;"));
    }

    //TO DO:  boolean operators
}

TEST_CASE("basic integer expressions") {

    SECTION("literal ints") {
        REQUIRE(test<int>("1;") == 1);
        REQUIRE(test<int>("-1;") == -1);
        REQUIRE(test<int>("2;") == 2);
        REQUIRE(test<int>("-2;") == -2);
    }

    SECTION("basic binary expressions for each operator") {
        REQUIRE(test<int>("2 + 3;") == 5);
        REQUIRE(test<int>("2 - 3;") == -1);
        REQUIRE(test<int>("2 * 3;") == 6);
        REQUIRE(test<int>("6 / 2;") == 3);
    }

    SECTION("Order of operations") {
        REQUIRE(test<int>("1 + 2 * 3;") == 7);
        REQUIRE(test<int>("6 + 10 / 2;") == 11);
        REQUIRE(test<int>("(1 + 2) * 3;") == 9);
        REQUIRE(test<int>("(5 + 10) / 3;") == 5);
    }
}

TEST_CASE("equality") {
    //Boolean
    REQUIRE(test<bool>("true == true;"));
    REQUIRE(test<bool>("false == false;"));
    REQUIRE(!test<bool>("false == true;"));
    REQUIRE(!test<bool>("true == false;"));

    //Integer
    REQUIRE(test<bool>("1 == 1;"));
    REQUIRE(!test<bool>("1 == 2;"));

    //Float
    REQUIRE(test<bool>("1.0 == 1.0;"));
    REQUIRE(!test<bool>("1.0 == 2.0;"));

    //Integer-float implict casts
    REQUIRE(test<bool>("1 == 1.0;"));
    REQUIRE(!test<bool>("1 == 2.0;"));
    REQUIRE(test<bool>("1.0 == 1;"));
    REQUIRE(!test<bool>("1.0 == 2;"));

    //Integer-bool implict casts?

    //Float-bool implict casts?

}

TEST_CASE("basic float expressions") {
    SECTION("literal floats") {
        REQUIRE(test<float>("1.0;") == 1.0);
        REQUIRE(test<float>("-1.0;") == -1.0);
        REQUIRE(test<float>("2.0;") == 2.0);

        REQUIRE(test<float>("234.0;") == 234.0);
        REQUIRE(test<float>("-2.0;") == -2);
    }
    SECTION("basic binary expressions for each operator") {
        REQUIRE(test<float>("2.0 + 3.0;") == 5.0);
        REQUIRE(test<float>("2.0 - 3.0;") == -1.0);
        REQUIRE(test<float>("2.0 * 3.0;") == 6.0);
        REQUIRE(test<float>("6.0 / 2.0;") == 3.0);
    }

    SECTION("Order of operations") {
        REQUIRE(test<float>("1.0 + 2.0 * 3.0;") == 7.0);
        REQUIRE(test<float>("6.0 + 10.0 / 2.0;") == 11.0);
        REQUIRE(test<float>("(1.0 + 2.0) * 3.0;") == 9.0);
        REQUIRE(test<float>("(5.0 + 10.0) / 3.0;") == 5.0);
    }
}

TEST_CASE("conditional expressions operator") {
    REQUIRE(test<int>("(? true, 1, 2);") == 1);
    REQUIRE(test<int>("(? false, 1, 2);") == 2);

    //Implicit casting of true part / false part
    REQUIRE(test<float>("(? true, 1.0, 2);") == 1.0);
    REQUIRE(test<float>("(? false, 1, 2.0);") == 2.0);

    //Implicit casting of condition from int to bool
    REQUIRE(test<int>("(? 1, 2, 3);") == 2);
    REQUIRE(test<int>("(? 2, 2, 3);") == 2);
    REQUIRE(test<int>("(? 0, 2, 3);") == 3);

    //Implicit casting of condition from float to bool
    REQUIRE(test<int>("(? 1.0, 2, 3);") == 2);
    REQUIRE(test<int>("(? 0, 2, 3);") == 3);

    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    REQUIRE(test<int>(ec, "foo:int = (? 1.0, 2, 3);") == 2);
    REQUIRE(test<int>(ec, "foo;") == 2);
    REQUIRE(test<int>(ec, "foo = (? 0, 2, 3);") == 3);
}

TEST_CASE("casting") {

    SECTION("implicit casts with literals") {
        //int to float
        REQUIRE(test<float>("1.0 + 1;") == 2.0);
        REQUIRE(test<float>("1 + 1.0;") == 2.0);
    }

    SECTION("implicit cast to bool type") {
        std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
        REQUIRE(!test<bool>(ec, "foo:bool;"));
        REQUIRE(!test<bool>(ec, "foo = 0;"));
        REQUIRE(test<bool>(ec, "foo = 1;"));

        REQUIRE(!test<bool>(ec, "foo = 0.0;"));
        REQUIRE(test<bool>(ec, "foo = 1.0;"));
    }

    //TODO: implicit casts with variables?

    SECTION("explicit casts") {
        //int to bool
        REQUIRE(!test<bool>("cast<bool>(0);"));
        REQUIRE(test<bool>("cast<bool>(1);"));
        REQUIRE(test<bool>("cast<bool>(2);"));

        //float to int
        REQUIRE(test<int>("cast<int>(1.0) + 1;") == 2);
        REQUIRE(test<int>("1 + cast<int>(1.0);") == 2);

        //float to bool
        REQUIRE(test<bool>("cast<bool>(2.1);"));
        REQUIRE(!test<bool>("cast<bool>(0.0);"));
    }
}

TEST_CASE("variable declarations") {
    SECTION("bool variable declarations") {
        REQUIRE(!test<bool>("foo:bool;"));
        REQUIRE(test<bool>("foo:bool = true;"));
    }

    SECTION("int variable declarations") {
        REQUIRE(test<int>("foo:int;") == 0);
        REQUIRE(test<int>("foo:int = 101;") == 101);
    }

    SECTION("float variable declarations") {
        REQUIRE(test<float>("foo:float;") == 0.0);
        REQUIRE(test<float>("foo:float = 101.0;") == 101.0);
    }

    SECTION("initializer casting") {
        REQUIRE(!test<bool>("foo:bool = cast<bool>(0);")); //explicit
        REQUIRE(test<bool>("foo:bool = cast<bool>(1);")); //explicit
        REQUIRE(test<bool>("foo:bool = cast<bool>(2);")); //explicit

        REQUIRE(test<float>("foo:float = 101;") == 101.0);          //implicit
        REQUIRE(test<int>("foo:int = cast<int>(101.0);") == 101.0); //explicit
    }
}

TEST_CASE("variables with initializers persist between REPL evaluations") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(test<bool>(ec, "fee:bool= true;"));
    REQUIRE(test<bool>(ec, "fee;"));
    REQUIRE(test<bool>(ec, "fee;"));

    REQUIRE(test<int>(ec, "foo:int = 123;") == 123);
    REQUIRE(test<int>(ec, "foo;") == 123);
    REQUIRE(test<int>(ec, "foo;") == 123);

    REQUIRE(test<float>(ec, "bar:float = 234.0;") == 234.0);
    REQUIRE(test<float>(ec, "bar;") == 234.0);
    REQUIRE(test<float>(ec, "bar;") == 234.0);
}

TEST_CASE("variables without initializers persist between REPL evaluations") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(!test<bool>(ec, "fee:bool;"));
    REQUIRE(!test<bool>(ec, "fee;"));
    REQUIRE(!test<bool>(ec, "fee;"));

    REQUIRE(test<int>(ec, "foo:int;") == 0);
    REQUIRE(test<int>(ec, "foo;") == 0);
    REQUIRE(test<int>(ec, "foo;") == 0);

    REQUIRE(test<float>(ec, "bar:float;") == 0);
    REQUIRE(test<float>(ec, "bar;") == 0);
    REQUIRE(test<float>(ec, "bar;") == 0);
}

TEST_CASE("chained variable declaration and assignment") {
    SECTION("int") {
        std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
        REQUIRE(test<int>(ec, "a:int = b:int = c:int = 10;"));
        REQUIRE(test<int>(ec, "a;") == 10);
        REQUIRE(test<int>(ec, "b;") == 10);
        REQUIRE(test<int>(ec, "c;") == 10);
    }
    SECTION("float") {
        std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
        REQUIRE(test<float>(ec, "a:float = b:float = c:float = 1.23;"));
        REQUIRE(test<float>(ec, "a;") == 1.23f);
        REQUIRE(test<float>(ec, "b;") == 1.23f);
        REQUIRE(test<float>(ec, "c;") == 1.23f);
    }
    SECTION("bool") {
        std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
        REQUIRE(test<bool>(ec, "a:bool = b:bool = c:bool = true;"));
        REQUIRE(test<bool>(ec, "a;"));
        REQUIRE(test<bool>(ec, "b;"));
        REQUIRE(test<bool>(ec, "c;"));
    }
}

TEST_CASE("multiple declarations in a module") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    auto results = testWithResults(ec, "foo:int = 10; bar:float = 1.1; bat:bool = true;");
    REQUIRE(results.size() == 3);
    REQUIRE(results[0].get<int>() == 10);
    REQUIRE(results[1].get<float>() == 1.1f);
    REQUIRE(results[2].get<bool>());
    REQUIRE(test<int>(ec, "foo;") == 10);
    REQUIRE(test<float>(ec, "bar;") == 1.1f);
    REQUIRE(test<bool>(ec, "bat;"));
}

TEST_CASE("arithmetic with variables") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(test<int>(ec, "foo:int = 100;") == 100);
    REQUIRE(test<int>(ec, "foo + 2;") == 102);
    REQUIRE(test<int>(ec, "foo - 2;") == 98);
    REQUIRE(test<int>(ec, "foo * 2;") == 200);
    REQUIRE(test<int>(ec, "foo / 2;") == 50);

    REQUIRE(test<float>(ec, "bar:float = 100.0;") == 100.0);
    REQUIRE(test<float>(ec, "bar + 2.0;") == 102.0);
    REQUIRE(test<float>(ec, "bar - 2.0;") == 98.0);
    REQUIRE(test<float>(ec, "bar * 2.0;") == 200.0);
    REQUIRE(test<float>(ec, "bar / 2.0;") == 50.0);
}

TEST_CASE("simple assignment expressions") {
    //TODO: bool
    //TODO: float
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    REQUIRE(test<int>(ec, "foo:int = 100;") == 100);
    REQUIRE(test<int>(ec, "foo;") == 100);
    REQUIRE(test<int>(ec, "foo = 200;") == 200);
    REQUIRE(test<int>(ec, "foo;") == 200);
    REQUIRE(test<int>(ec, "foo = 201;") == 201);
    REQUIRE(test<int>(ec, "foo;") == 201);
}

TEST_CASE("chained assignment expressions") {
    //TODO: bool
    //TODO: float
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    REQUIRE(test<int>(ec, "foo:int;") == 0);
    REQUIRE(test<int>(ec, "bar:int;") == 0);
    REQUIRE(test<int>(ec, "bat:int;") == 0);
    REQUIRE(test<int>(ec, "foo = bar = 101; ") == 101);
    REQUIRE(test<int>(ec, "foo = bar = bat = 101; ") == 101);
}

TEST_CASE("variable declaration as expression") {
    //TODO: bool
    //TODO: float
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    REQUIRE(test<int>(ec, "foo:int = 100;") == 100);
    REQUIRE(test<int>(ec, "foo = bar:int = 102;") == 102);
    REQUIRE(test<int>(ec, "foo;") == 102);
}