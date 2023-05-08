add_library(shadertools)

target_compile_features(shadertools PRIVATE cxx_std_20)
target_include_directories(shadertools PUBLIC ${SHADER_PROCESSOR_INCLUDE_DIR})

target_sources(shadertools PRIVATE shader_binary.cpp
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/shader_binary.hpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/shader_constants.hpp")

add_executable(shaderprocessor)

if(TARGET glslang)
    target_compile_definitions(shaderprocessor PRIVATE WITH_GLSLANG_SHADERS)
    target_link_libraries(shaderprocessor PUBLIC glslang SPIRV)
    target_sources(shaderprocessor PRIVATE "compile_glsl.cpp")
endif()

if(TARGET slang::core)
    target_compile_definitions(shaderprocessor PRIVATE WITH_SLANG_SHADERS)
    target_link_libraries(shaderprocessor PUBLIC slang::core)
    target_sources(shaderprocessor PRIVATE "compile_slang.cpp")
endif()

if(TARGET spirv-cross-core)
    target_compile_definitions(shaderprocessor PRIVATE WITH_SPIRV_CROSS)
    target_link_libraries(shaderprocessor PUBLIC spirv-cross-core spirv-cross-c)
    target_sources(shaderprocessor PRIVATE "compile_spirv_cross.cpp")
endif()

if(APPLE)
    find_library(FOUNDATION_LIBRARY Foundation)
    target_link_libraries(shaderprocessor PRIVATE ${FOUNDATION_LIBRARY})
    target_sources(shaderprocessor PRIVATE "compile_msl.mm")
endif()

target_link_libraries(shaderprocessor PUBLIC shadertools simdjson magic_enum::magic_enum)
target_compile_features(shaderprocessor PRIVATE cxx_std_20)
target_include_directories(shaderprocessor PUBLIC "${SHADER_PROCESSOR_INCLUDE_DIR}")

target_sources(shaderprocessor PRIVATE "shader_json.cpp" "shader_processor.cpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/shader_binary.hpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/shader_constants.hpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/glslang_resource.hpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/compile.hpp"
    "${SHADER_PROCESSOR_INCLUDE_DIR}/shaders/shader_json.hpp")
