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

# This will now search for JSONs in the "shaders" directory, generate targets for them, and add them
# as a dependency of the "my_game" target.
create_shader_targets(shaders my_game)
```

Everytime you modify one of the JSONs or the shaders they will automatically be rebuilt and packaged when
building the project.

## Supported compilers
- [glslang](https://github.com/KhronosGroup/glslang)
- [slangc](https://github.com/shader-slang/slang)

Support for HLSL, Metal, and SPIRV-Cross will be added in the future.

## TODOs
- Configurable output directory
- Shader compression (LZMA?)
- Support for multiple output directories and targets
- Better stage resolution. (Input GLSL and target MSL should create a chain of GLSL -> SPIR-V -> MSL)
