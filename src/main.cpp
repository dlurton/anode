#include "parse.h"
#include "execute.h"
#include "visualize.h"

#include <linenoise.h>

#include <cstring>



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
    void executeLine(std::string lineOfCode, bool shouldCompile);

}

int main(int argc, char **argv) {

    lwnn::execute::initializeJitCompiler();

    linenoiseInstallWindowChangeHandler();

    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        }
    }

    const char* historyFilename = "~/.lwnn_history";

    linenoiseHistoryLoad(historyFilename);
    //linenoiseSetCompletionCallback(completionHook);

    printf("starting...\n");

    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";

    bool keepGoing = true;
    bool shouldCompile = true;

    while (keepGoing) {
        char* lineOfCode = linenoise(prompt);

        if (lineOfCode == NULL) {
            break;
        } else if (!strncmp(lineOfCode, "/c", 8)) {
            shouldCompile = !shouldCompile;
            std::cout << "Compilation " << (shouldCompile ? "enabled" : "disabled") << "\n";
        } else if (!strncmp(lineOfCode, "/history", 8)) {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        } else if (*lineOfCode == '\0') {
            keepGoing = false;
        } else {
            linenoiseHistoryAdd(lineOfCode);

            lwnn::executeLine(lineOfCode, shouldCompile);
        }

        free(lineOfCode);
        linenoiseHistorySave(historyFilename);
    }

    printf("Saving history...");
    linenoiseHistoryFree();

    return 0;
}

namespace lwnn {
    std::string run(std::unique_ptr<const ast::Expr> expr, bool enableCompilation) {
        const char *FUNC_NAME = "someFunc";
        ast::DataType exprDataType = expr->dataType();

        std::unique_ptr<const ast::Return> retExpr = std::make_unique<const ast::Return>(ast::SourceSpan::Any, std::move(expr));
        ast::FunctionBuilder fb{ ast::SourceSpan::Any, FUNC_NAME, retExpr->dataType() };

        ast::BlockExprBuilder &bb = fb.blockBuilder();

        bb.addExpression(std::move(retExpr));

        ast::ModuleBuilder mb{"someModule"};

        mb.addFunction(fb.build());

        std::unique_ptr<const ast::Module> lwnnModule{mb.build()};

        visualize::prettyPrint(lwnnModule.get());

        if(!enableCompilation) {
            return "";
        }

        auto ec = execute::createExecutionContext();
        ec->addModule(lwnnModule.get());

        uint64_t funcPtr = ec->getSymbolAddress(FUNC_NAME);

        std::string resultAsString;
        switch(exprDataType) {
            case ast::DataType::Int32: {
                execute::IntFuncPtr intFuncPtr = reinterpret_cast<execute::IntFuncPtr>(funcPtr);
                int result = intFuncPtr();
                resultAsString = std::to_string(result);
                break;
            }
            default:
                throw exception::UnhandledSwitchCase();
        }

        return resultAsString;
    }

    void executeLine(std::string lineOfCode, bool enableCompilation) {
        auto expr = lwnn::parse::parseString(lineOfCode);

        std::string result = run(std::move(expr), enableCompilation);
        if(enableCompilation) {
            std::cout << "Result: " << result << "\n";
        }
    }
}