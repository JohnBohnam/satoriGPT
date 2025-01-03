1. install ollama `curl -fsSL https://ollama.com/install.sh | sh` - check the [ollama github](https://github.com/ollama/ollama?tab=readme-ov-file) for more information.
2. run `ollama run codellama` to start the ollama server
3. download the ollama-hpp header file from the [ollama-hpp github](https://github.com/jmont-dev/ollama-hpp)
4.run with 
```
g++ main.cpp -I[path to ollama-hpp]/singleheader -o main
./main
```
