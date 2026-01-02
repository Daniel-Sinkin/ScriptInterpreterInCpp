# Building the Project
## Debug Build
```
cmake -S ds_lang -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```
## Release Build
```
cmake -S ds_lang -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
## Creating and running tests
```
cmake -S ds_lang -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```