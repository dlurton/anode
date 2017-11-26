

#include "anode.h"
#include "front/parse.h"
#include "front/ast_passes.h"

#include <iostream>
#include <fstream>
#include <front/ErrorStream.h>


using namespace anode;
using namespace anode::front;

int failCount = 0;
void fail(const std::string &filename, int lineNum, const std::string &message) {
    std::cout << filename << ":" << lineNum << " FAILED: " << message << "\n";
    failCount++;
}

int passCount = 0;
void pass(const std::string &filename, int lineNum) {
    std::cout << filename << ":" << lineNum << " PASS\n";
    passCount++;
}

int ignoreCount = 0;
void ignore(const std::string &filename, int lineNum) {
    std::cout << filename << ":" << lineNum << " IGNORED\n";
    ignoreCount++;
}

std::string formatErrorDetails(std::string message, std::string source, std::string errorOutput) {
    std::string details = message + ":\nParsed source:\n" + source + "\nError Output:\n";
    if(errorOutput.size() == 0) {
        details += "(no error output)\n";
    } else {
        details += errorOutput + "\n";
    }
    return details;
}

int main(int argc, char **argv) {
    GC_INIT();
    if(argc <= 1) {
        std::cout << "Missing argument: <path to negative test case file>\n";
    } else if(argc > 2) {
        std::cout << "Only one argument expected\n";
    }

    std::string filename{argv[1]};
    std::ifstream inputFileStream{filename};
    if(!inputFileStream) {
        std::cout << "Couldn't open input file: " << filename;
        return -1;
    }

    std::string line;
    std::string sourceBuilder;
    int startingLine = 1;
    for(int lineNum = 1; inputFileStream; ++lineNum) {
        std::getline(inputFileStream, line);
        if(!inputFileStream) {
            break;
        }

        if(line.size() > 1 && line[0] == '?') {
            if (line.size() > 2 && line[1] == '#') {
                ignore(filename, lineNum);
                sourceBuilder = "";
                continue;
            }
            std::string testCaseSource = sourceBuilder;
            std::string source = sourceBuilder;
            sourceBuilder = "";
            startingLine = lineNum;

            auto parts = string::split(line.substr(1, line.size() - 1), ",");
            if (parts.size() != 3) {
                fail(filename, lineNum, "'expected' line should have 3 parts: ErrorKind,lineNum,charIndex");
                continue;
            }

            std::string expectedErrorCodeEnumName = string::trim_copy(parts[0]);
            long expectedCharIndex, expectedLineNum;
            try {
                expectedCharIndex = std::stol(parts[2].c_str());
                expectedLineNum = std::stol(parts[1].c_str());
            } catch(std::invalid_argument &) {
                fail(filename, lineNum, "Invalid line number or columnm");
            } catch(std::out_of_range &){
                fail(filename, lineNum, "Line number or column is out of range");
            }

            if (!error::ErrorKind::_is_valid(expectedErrorCodeEnumName.c_str())) {
                fail(filename, lineNum, "Invalid ErrorKind:  " + expectedErrorCodeEnumName);
                continue;
            }

            error::ErrorKind expectedErrorKind = error::ErrorKind::_from_string(expectedErrorCodeEnumName.c_str());

            std::stringstream stream{source};
            std::stringstream errorOutput;
            error::ErrorStream errorStream{errorOutput};
            try {
                auto &&module = parseModule(stream, string::format("<test case ending at %s:%d>", filename.c_str(), startingLine), errorStream);
                if (errorStream.errorCount() == 0) {
                    ast::AnodeWorld world;
                    try {
                        front::passes::runAllPasses(world, module, errorStream);
                    } catch(anode::exception::DebugAssertionFailedException &de) {
                        std::string msg = std::string("Debug assertion failed:\n\t") + de.what();
                        fail(filename, lineNum, formatErrorDetails(msg, source, errorOutput.str()));
                        continue;
                    }
                }
            }
            catch (ParseAbortedException ex) {}

            error::ErrorDetail error = errorStream.firstError();
            if (errorStream.errorCount() == 0) {
                fail(filename, lineNum, formatErrorDetails("Expected an error but there wasn't an error", source, errorOutput.str()));
                continue;
            }

            if (error.errorKind != expectedErrorKind) {
                auto message = string::format("Expected ErrorKind::%s but was ErrorKind::%s",
                                              expectedErrorKind._to_string(),
                                              errorStream.firstError().errorKind._to_string());

                auto details = formatErrorDetails(message, source, errorOutput.str());

                fail(filename, lineNum, details);
                continue;
            }

            if (error.sourceSpan.start().line() != expectedLineNum || error.sourceSpan.start().position() != expectedCharIndex) {
                auto message = formatErrorDetails(
                    string::format("Expected first error at (%d, %d) but was at (%d, %d)",
                                   expectedLineNum,
                                   expectedCharIndex,
                                   error.sourceSpan.start().line(),
                                   error.sourceSpan.start().position()), source,
                                   errorOutput.str());

                fail(filename, lineNum, message);
                continue;
            }

            if (error.sourceSpan.start().position() != expectedCharIndex) {
                fail(filename, lineNum, formatErrorDetails(
                    string::format("Expected first error on line %d but was on line %d", expectedLineNum, error.sourceSpan.start().position()), source,
                    errorOutput.str()));
                continue;
            }

            pass(filename, lineNum);
        } else {
            sourceBuilder += line + "\n";
        }
    }
    std::cout << "****************************************************************************\n";
    std::cout << "Cases passed: " << passCount << ", cases ignored: " << ignoreCount << ", cases failed: " << failCount << "\n";
    std::cout << "****************************************************************************\n";
    if(failCount > 0) {

         return -1;
    }
}