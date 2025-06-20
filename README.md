# cuBoolGraph
cuBool based graph analysis algorithms

## List of algorithms

regular_path_query - Single-Source Regular Path Query Algorithm in Terms of Linear Algebra

# Run tests
cuBoolGraph supports Linux-based OS (tested on Ubuntu 24.04 and Manjaro 6.19).
For building project are required gcc version 14 and later, CMake with version 3.25 and later, CUDA developing toolkint version 12.0 and later. 

For build and run tests execute commands
```
git submodule update --init --recursive
cmake -B build -DCUBOOL_GRAPH_ENABLE_TESTING=ON
cmake --build build
./build/tests/cuboolgraph_tests
```
