#include "parse.h"
#include "ast_passes.h"
#include "execute.h"
#include "visualize.h"
#include "error.h"
#include "backtrace.h"
#include "string.h"

#include <unistd.h>

#include <linenoise.h>
#include <cstring>


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
    void evaluateLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName, bool shouldExecute);
}

std::string getHistoryFilePath() {
    std::string home{getenv("HOME")};
    return home + "/.lwnn_history";
}

std::string readLineOfCode() {
    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";
    char* lineOfCodeChar = linenoise(prompt);
    if (lineOfCodeChar == NULL) {
        return "";
    }
    std::string lineOfCode{lineOfCodeChar};
    free(lineOfCodeChar);
    return lineOfCode;
}

void help() {
    std::cout << "Command       Description\n";
    std::cout << "/help         Displays this text.\n";
    std::cout << "/compile      Toggles compilation.  When disabled, the LLVM IR will not be generated.\n";
    std::cout << "/history      Displays command history.\n";
    std::cout << "/exit         Exits the lwnn REPL.\n\n";
    std::cout << "Valid lwnn statements may also be entered.";
}

int main(int argc, char **argv) {
    if(!isatty(fileno(stdin))) {
        std::cout << "stdin is not a terminal\n";
        return -1;
    }
        
    //lwnn::backtrace::initBacktraceDumper();
    linenoiseInstallWindowChangeHandler();
    int commandCount = 1;

    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        }
    }

    const std::string historyFilename = getHistoryFilePath();

    linenoiseHistoryLoad(historyFilename.c_str());

    //linenoiseSetCompletionCallback(completionHook);

    std::shared_ptr<lwnn::execute::ExecutionContext> executionContext = lwnn::execute::createExecutionContext();

    std::cout << "Welcome to the lwnn REPL.  Type '/help' for help or '/exit' to exit.\n";

    bool keepGoing = true;
    bool shouldCompile = true;

    while (keepGoing) {
        std::string lineOfCode{readLineOfCode()};
        linenoiseHistoryAdd(lineOfCode.c_str());
        if (lineOfCode == "/compile") {
            shouldCompile = !shouldCompile;
//             std::cout << "Compilation " << (shouldCompile ? "enabled" : "disabled") << "\n";
        }
        else if(lineOfCode == "/help") {
            help();
        }
        else if (lineOfCode == "/exit") {
            keepGoing = false;
        }
        else if (lineOfCode == "/history") {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        }
        else {
            std::string moduleName = lwnn::string::format("<repl_line_%d>", ++commandCount);
            lwnn::evaluateLine(executionContext, lineOfCode, moduleName, shouldCompile);
        }

        linenoiseHistorySave(historyFilename.c_str());
    }

    linenoiseHistoryFree();
    return 0;
}

namespace lwnn {
    bool runModule(std::shared_ptr<execute::ExecutionContext> executionContext,
                   std::unique_ptr<ast::Module> lwnnModule,
                   std::string &resultAsString) {
        ASSERT(executionContext);
        ASSERT(lwnnModule);
        executionContext->setPrettyPrintAst(true);
        executionContext->setDumpIROnLoad(true);

        try {
            executionContext->prepareModule(lwnnModule.get());
        } catch(execute::ExecutionException &e) {
            return false;
        }

        auto bodyExpr = dynamic_cast<ast::ExprStmt*>(lwnnModule->body());
        if(!bodyExpr) {
            executionContext->executeModule(std::move(lwnnModule));
            return false;
        } else {
            auto moduleInitResultType = bodyExpr->type();
            ASSERT(moduleInitResultType->isPrimitive() && "Non-primitive types not yet supported here");
            try {
                switch (moduleInitResultType->primitiveType()) {
                    case type::PrimitiveType::Int32: {
                        int result = executionContext->executeModuleWithResult<int>(std::move(lwnnModule));
                        resultAsString = std::to_string(result);
                        return true;
                    }
                    case type::PrimitiveType::Float: {
                        float result = executionContext->executeModuleWithResult<float>(std::move(lwnnModule));
                        resultAsString = std::to_string(result);
                        return true;
                    }
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            } catch(execute::ExecutionException ec) {
                return false;
            }
        }
    }

    void evaluateLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName, bool shouldExecute) {
        std::unique_ptr<lwnn::ast::Module> module = lwnn::parse::parseModule(lineOfCode, moduleName);
        //If no Module returned, parsing failed.
        if(!module) {
            return;
        }

        if(shouldExecute) {
            std::string resultAsString;
            bool hasResult = runModule(executionContext, std::move(module), resultAsString);
            std::cout << (hasResult ? resultAsString : "<no result>") << "\n";
        }
    } // evaluateLine
} //namespace lwnn
