# shader_processor

A CMake library that automatically compiles and packages your shaders into binary files. Using JSON,
you can define how your shaders will be packaged and optionally compiled using glslang, slangc, spirv-cross,
and Metal.

The library is written in C++20 and currently depends on [magic_enum](https://github.com/Neargye/magic_enum)
and [simdjson](https://github.com/simdjson/simdjson).

## Usage

In your CMakeLists.txt, you need to call some functions of the library.

```cmake
add_executable(my_game "game.cpp")

create_shader_targets(shaders)
add_shader_target_dependency(my_game)
```

Everytime you modify one of the JSONs or the shaders they will automatically be rebuilt and packaged when
building the project.

## Supported compilers
- [glslang](https://github.com/KhronosGroup/glslang)
- [slangc](https://github.com/shader-slang/slang)
- [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross)
- Metal

## TODOs
- Configurable output directory
- Shader compression (LZMA?)
- Support for multiple output directories and targets
- Per-JSON targets to only recompile affected shaders
