#include "parse.h"
#include "ExecutionContext.h"
#include "PrettyPrinter.h"
#include "AstWalker.h"

#include <linenoise.h>

#include <string>

#include <cstring>
#include <cstdlib>
#include <cstdio>


//
//static const char* examples[] = {
//        "db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
//};
//
//void completionHook (char const* prefix, linenoiseCompletions* lc) {
//    size_t i;
//
//    for (i = 0;  examples[i] != NULL; ++i) {
//        if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
//            linenoiseAddCompletion(lc, examples[i]);
//        }
//    }
//}



namespace lwnn {
    extern void do_llvm_demo();
    void executeLine(std::string lineOfCode);

}

int main(int argc, char **argv) {

    lwnn::initializeJitCompiler();

    linenoiseInstallWindowChangeHandler();

    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        }
    }

    const char* file = "~/.lwnn_history";

    linenoiseHistoryLoad(file);
    //linenoiseSetCompletionCallback(completionHook);

    printf("starting...\n");

    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";

    bool keepGoing = true;
    while (keepGoing) {
        char* lineOfCode = linenoise(prompt);

        if (lineOfCode == NULL) {
            break;
        } else if (!strncmp(lineOfCode, "/history", 8)) {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        }
        if (*lineOfCode == '\0') {
            keepGoing = false;
        } else {
            linenoiseHistoryAdd(lineOfCode);

            lwnn::executeLine(lineOfCode);
        }

        free(lineOfCode);
        linenoiseHistorySave(file);
    }

    printf("Saving history...");
    linenoiseHistoryFree();

    return 0;
}

namespace lwnn {

    std::string compile(std::unique_ptr<const Expr> expr) {
        const char *FUNC_NAME = "someFunc";
        DataType exprDataType = expr->dataType();

        std::unique_ptr<const Return> retExpr = std::make_unique<const Return>(SourceSpan::Any, std::move(expr));
        FunctionBuilder fb{ SourceSpan::Any, FUNC_NAME, retExpr->dataType() };

        BlockExprBuilder &bb = fb.blockBuilder();

        bb.addExpression(std::move(retExpr));

        ModuleBuilder mb{"someModule"};

        mb.addFunction(fb.build());

        std::unique_ptr<const Module> lwnnModule{mb.build()};

        prettyPrint(lwnnModule.get());


        std::unique_ptr<ExecutionContext> ec = createExecutionContext();
        ec->addModule(lwnnModule.get());

        uint64_t funcPtr = ec->getSymbolAddress(FUNC_NAME);

        std::string resultAsString;
        switch(exprDataType) {
            case DataType::Int32: {
                IntFuncPtr intFuncPtr = reinterpret_cast<IntFuncPtr>(funcPtr);
                int result = intFuncPtr();
                resultAsString = std::to_string(result);
                break;
            }
            default:
                throw UnhandledSwitchCase();
        }

        return resultAsString;
    }

    void executeLine(std::string lineOfCode) {
        auto expr = lwnn::parse(lineOfCode);

        std::string result = compile(std::move(expr));
        std::cout << "Result: " << result << "\n";
    }
}