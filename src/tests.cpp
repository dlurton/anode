
#define VISUALIZE_AST
#define DUMP_IR

#include "execute.h"
#include "parse.h"
#include "compile.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace lwnn;

int testCount = 0;

template<typename T>
T test(std::shared_ptr<execute::ExecutionContext> executionContext, std::string source) {
    std::string module_name = string::format("test_%d", ++testCount);

    std::unique_ptr<ast::Module> module = parse::parseModule(source, module_name);
    executionContext->prepareModule(module.get());

#ifdef VISUALIZE_AST
    executionContext->setPrettyPrintAst(true);
#endif

#ifdef DUMP_IR
    executionContext->setDumpIROnLoad(true);
#endif

    T result = executionContext->executeModuleWithResult<T>(std::move(module));

    return result;
}

template<typename T>
T test(std::string source) {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();
    T result = test<T>(ec, source);
    return result;
}


// catch.hpp was intended to be used with a different pattern...

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

TEST_CASE("basic float expressions") {
    SECTION("literal floats") {
        REQUIRE(test<float>("1.0;") == 1.0);
        REQUIRE(test<float>("-1.0;") == -1);
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

TEST_CASE("casting") {
    SECTION("implicit casts") {
        REQUIRE(test<float>("1.0 + 1;") == 2.0);
        REQUIRE(test<float>("1 + 1.0;") == 2.0);
    }

    SECTION("explicit casts") {
        REQUIRE(test<int>("cast<int>(1.0) + 1;") == 2);
        REQUIRE(test<int>("1 + cast<int>(1.0);") == 2);
    }
}

TEST_CASE("variable declarations") {
    SECTION("int variable declarations") {
        REQUIRE(test<int>("foo:int;") == 0);
        REQUIRE(test<int>("foo:int = 101;") == 101);
    }

    SECTION("float variable declarations") {
        REQUIRE(test<float>("foo:float;") == 0.0);
        REQUIRE(test<float>("foo:float = 101.0;") == 101.0);
    }

    SECTION("initializer casting") {
        REQUIRE(test<float>("foo:float = 101;") == 101.0);          //implicit
        REQUIRE(test<int>("foo:int = cast<int>(101.0);") == 101.0); //explicit
    }
}

TEST_CASE("variables with initializers persist between REPL evaluations") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(test<int>(ec, "foo:int = 123;") == 123);
    REQUIRE(test<int>(ec, "foo;") == 123);
    REQUIRE(test<int>(ec, "foo;") == 123);

    REQUIRE(test<float>(ec, "bar:float = 234.0;") == 234.0);
    REQUIRE(test<float>(ec, "bar;") == 234.0);
    REQUIRE(test<float>(ec, "bar;") == 234.0);

}

TEST_CASE("variables without initializers persist between REPL evaluations") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(test<int>(ec, "foo:int;") == 0);
    REQUIRE(test<int>(ec, "foo;") == 0);
    REQUIRE(test<int>(ec, "foo;") == 0);

    REQUIRE(test<float>(ec, "bar:float;") == 0);
    REQUIRE(test<float>(ec, "bar;") == 0);
    REQUIRE(test<float>(ec, "bar;") == 0);

}

TEST_CASE("arithmatic with variables") {
    std::shared_ptr<execute::ExecutionContext> ec = execute::createExecutionContext();

    REQUIRE(test<int>(ec, "foo:int = 100;") == 100);
    REQUIRE(test<int>(ec, "foo + 2;") == 102);
    REQUIRE(test<int>(ec, "foo - 2;") == 98);
    REQUIRE(test<int>(ec, "foo * 2;") == 200);
    REQUIRE(test<int>(ec, "foo / 2;") == 50);

    REQUIRE(test<float>(ec, "bar:float = 100.0;") == 100.0);
    REQUIRE(test<float>(ec, "bar + 2;") == 102.0);
    REQUIRE(test<float>(ec, "bar - 2;") == 98.0);
    REQUIRE(test<float>(ec, "bar * 2;") == 200.0);
    REQUIRE(test<float>(ec, "bar / 2;") == 50.0);
}