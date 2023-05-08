cmake_minimum_required(VERSION 3.11)
project(shader_processor)

set(SHADER_PROCESSOR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SHADER_PROCESSOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src)

macro(create_shader_targets SHADER_DIRECTORY)
    # Search for JSONs in the shaders directory.
    file(GLOB_RECURSE SHADER_JSONS "${SHADER_DIRECTORY}/*.json" "${SHADER_DIRECTORY}/**/*.json")
    file(GLOB_RECURSE SHADER_FILES "${SHADER_DIRECTORY}/*")

    # Create a target that depends on the shaders and shader jsons which has to be built before
    # the game target, in which we execute the shader processor. By creating a build_shaders.timestamp
    # file, we can track the time of the last build to avoid unnecessary recompilations of the shaders.
    add_custom_command(
        OUTPUT build_shaders.timestamp
        COMMAND ./shaders/shaderprocessor ${SHADER_JSONS}
        COMMAND ${CMAKE_COMMAND} -E touch build_shaders.timestamp
        DEPENDS ${SHADER_FILES} ${SHADER_JSONS}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        VERBATIM
    )
    add_custom_target(build_shaders DEPENDS build_shaders.timestamp)
    add_dependencies(build_shaders shaderprocessor)
endmacro()

macro(add_shader_target_dependency TARGET)
    add_dependencies(${TARGET} shaderprocessor build_shaders)
endmacro()
