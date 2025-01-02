#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <set>

namespace fs = std::filesystem;

std::string getUsersProblemDescription() {
    std::cout<<"type in path of the problem description:\n";
    std::string filePath;
    std::getline(std::cin, filePath);

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
    Correct, Incorrect, CompilationFailed, RunFailed
};

// returns the address of the file with the solution
std::string mockCreateCorrectSolution(std::string problemDescription) {
    const std::string solutionPath = "./solution.cpp";
    std::ofstream file(solutionPath);
    file<<"int x;" << std::endl;
    file<<"#include<bits/stdc++.h>" << std::endl;
    file<<"using namespace std;" << std::endl;
    file<<"int main() {" << std::endl;
    file<<"cin>>x;" << std::endl;
    file<<"cout<<x<<endl;" << std::endl;
    file<<"}" << std::endl;
    file.close();
    return solutionPath;
}

std::string getUsersPathToTestDir() {
    std::cout<<"type in path to the test directory:\n";
    std::string pathToTestDir;
    std::cin>>pathToTestDir;
    return pathToTestDir;
}

std::string changeExtension(std::string path, int extensionLength, std::string newExtension) {
    return path.substr(0, path.size() - extensionLength) + newExtension;
}

TestResult testSolution(std::string pathToSolution) {
    std::string pathToCompiledSolution = changeExtension(pathToSolution, 4, "");
    const std::string compilationCommand = "g++ " + pathToSolution + " -o " + pathToCompiledSolution;
    int compilationResult = system(compilationCommand.c_str());
    if (compilationResult != 0) {
        return CompilationFailed;
    }
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
                changeExtension(path.string(), 3, ".out");
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

int main() {
    std::string problemDescription = getUsersProblemDescription();

    if (problemDescription.empty()) {
        std::cout<<"Couldn't read problem description!"<<std::endl;
        return 1;
    }

    std::string pathToSolution = mockCreateCorrectSolution(problemDescription);

    if (pathToSolution.empty()) {
        std::cout<<"Couldn't generate solution";
        return 1;
    }

    TestResult testResult = testSolution(pathToSolution);

    std::cout<<testResult<<std::endl;
    return 0;
}
