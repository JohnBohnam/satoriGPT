#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <set>
#include "ollama.hpp"

namespace fs = std::filesystem;

std::string getUsersProblemDescription() {
    // std::cout<<"type in path of the problem description:\n";
    // std::string filePath;
    // std::getline(std::cin, filePath);
    std::string filePath = "problem.txt";

    std::ifstream file(filePath);
    if (!file) {
        std::cerr<<"Error: Could not open file at"<<filePath<<std::endl;
        return "";
    }

    std::string fileContents;
    std::string line;
    while (std::getline(file, line)) {
        fileContents += line + '\n';
    }
    file.close();

    return fileContents;
}

enum TestResult {
    Correct, Incorrect, RunFailed
};

enum CompilationResult {
    CompilationSuccess, CompilationFailed
};

class Assistant {

    std::string model;
    ollama::response context;
    std::ofstream solutionFile;

    std::function<void(const ollama::response&)> printPartialResponse = [this](const ollama::response &response ) {
        std::cout<<response.as_simple_string();
        fflush(stdout);
        solutionFile << response.as_simple_string();
    };

public:
    std::string solution_path;

    Assistant(std::string solution_path = "solution.cpp",
              std::string model = "codellama") : solution_path(solution_path), model(model) {
        // solutionFile.open(solution_path);
    }

    void prompt(std::string prompt) {
        solutionFile.open(solution_path);
        ollama::generate(model, prompt, context, printPartialResponse);
        solutionFile.flush();
        solutionFile.close();
    }

    ~Assistant() {
        // solutionFile.close();
    }
};

std::string getUsersPathToTestDir() {
    // std::cout<<"type in path to the test directory:\n";
    // std::string pathToTestDir;
    // std::cin>>pathToTestDir;
    // return pathToTestDir;
    return "tests";
}

std::string changeExtension(std::string path, int extensionLength, std::string newExtension) {
    return path.substr(0, path.size() - extensionLength) + newExtension;
}

CompilationResult compileSolution(std::string pathToSolution, std::string compileErrorsPath, std::string pathToCompiledSolution) {
    // std::cout << "Compiling solution..." << std::endl;
    // std::cout << "Path to solution: " << pathToSolution << std::endl;
    // std::cout << "The solution: " << std::endl;
    // std::string cat_command = "cat " + pathToSolution;
    // system(cat_command.c_str());
    // std::cout << std::endl << std::endl;

    const std::string compilationCommand = "g++ " + pathToSolution + " -o " + pathToCompiledSolution + " 2> " + compileErrorsPath;

    int compilationResult = system(compilationCommand.c_str());
    if (compilationResult != 0) {
        return CompilationFailed;
    }
    return CompilationSuccess;
}

TestResult testSolution(std::string pathToCompiledSolution, std::string pathToDiffOutput = "diffOutput.txt") {
    std::string pathToTestDir = getUsersPathToTestDir();
    std::set<fs::path> testInputs;
    std::set<fs::path> testOutputs;
    for (const auto &entry: fs::directory_iterator(pathToTestDir)) {
        std::cout<< entry.path() << " " << entry.path().extension().string() << " "<< entry.path().filename().string()<<std::endl;
        if (entry.path().extension().string() == ".in") {
            testInputs.insert(entry.path());
        }
    }

    int testsPassed = 0;

    for (auto path: testInputs) {
        std::string runCommand = "./" + pathToCompiledSolution + " < " + path.string() + " > " + "SatoriGPToutput.out";
        int runResult = system(runCommand.c_str());
        if (runResult != 0) {
            return RunFailed;
        }

        std::string diffCommand = "diff SatoriGPToutput.out " +
                changeExtension(path.string(), 3, ".out") + " > " + pathToDiffOutput;
        int filesAreDifferent = system(diffCommand.c_str());
        if (filesAreDifferent) {
            std::cout<<"Test "<<path.filename().string()<<"\033[31m"<<" FAILED"<<std::endl;
            std::cout<<"\033[0m"; // reset text color
            return Incorrect;
        }

        std::cout<<"Test "<<path.filename().string()<<"\033[32m"<<" PASSED"<<std::endl;
        std::cout<<"\033[0m"; // reset text color

        testsPassed++;
    }

    return Correct;

}

// remove everything before the first ``` and after the last ``` if there are strays
void destray(std::string filename) {
    std::ifstream file(filename);
    std::string fileContents;
    std::string line;
    bool foundFirst = false;
    while (std::getline(file, line)) {
        if (line.find("```") != std::string::npos) {
            if (!foundFirst) {
                foundFirst = true;
            } else {
                break;
            }
        }
        else if (foundFirst) {
            fileContents += line + '\n';
        }
    }

    file.close();
    std::ofstream newFile(filename);
    newFile << fileContents;
    newFile.close();
}

int main() {
    std::string problemDescription = getUsersProblemDescription();

    std::string onlyCodePrompt = " Write only the c++ code that will compile.";

    if (problemDescription.empty()) {
        std::cout<<"Couldn't read problem description!"<<std::endl;
        return 1;
    }

    std::string pathToSolution = "solution.cpp";
    std::string compileErrorsPath = "compileErrors.txt";
    std::string pathToCompiledSolution = "solution";
    std::string pathToDiffOutput = "diffOutput.txt";
    Assistant assistant(pathToSolution, "codellama");

    assistant.prompt(problemDescription + onlyCodePrompt);
    destray(pathToSolution);

    while (true) {

        CompilationResult compilationResult = compileSolution(pathToSolution, compileErrorsPath, pathToCompiledSolution);

        if (compilationResult == CompilationFailed) {
            std::cout<<"Compilation failed. Prompting compile errors."<<std::endl;
            std::string compileErrors;
            std::ifstream file(compileErrorsPath);
            if (file) {
                std::string line;
                while (std::getline(file, line)) {
                    compileErrors += line + '\n';
                }
                file.close();
            }
            std::cout << compileErrors << std::endl;
            assistant.prompt(compileErrors + onlyCodePrompt);
            destray(pathToSolution);
        } else {
            std::cout << "Compilation successful." << std::endl;
            TestResult testResult = testSolution(pathToCompiledSolution, pathToDiffOutput);
            std::string diffOutput;
            std::cout << "Test result: ";
            if (testResult == Correct) {
                std::cout << "Correct" << std::endl;
                return 0;
            } else if (testResult == Incorrect) {
                std::cout << "Incorrect" << std::endl;
                std::ifstream file(pathToDiffOutput);
                if (file) {
                    std::string line;
                    while (std::getline(file, line)) {
                        diffOutput += line + '\n';
                    }
                    file.close();
                }
                std::string prompt = "wrong answer." + diffOutput + onlyCodePrompt;
                std::cout << diffOutput << std::endl;
                assistant.prompt(prompt);
                destray(pathToSolution);
            } else if (testResult == RunFailed) {
                std::cout << "Run failed" << std::endl;
                assistant.prompt("Run failed");
                destray(pathToSolution);
            }
        }
    }
}
