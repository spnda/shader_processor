cmake_minimum_required(VERSION 3.11)
project(shader_processor)

set(SHADER_PROCESSOR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SHADER_PROCESSOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src)

macro(create_shader_targets SHADER_DIRECTORY TARGET_DEPENDENCY)
    # Search for JSONs in the shaders directory.
    file(GLOB_RECURSE SHADER_JSONS "${SHADER_DIRECTORY}/*.json" "${SHADER_DIRECTORY}/**/*.json")
    if(${CMAKE_VERSION} VERSION_GREATER "3.20.0")
        # CMake 3.19 added support for parsing JSONs, which we use to create single targets for each JSON.
        # We'll also use cmake_path here, which came with 3.20.
        foreach(SHADER_JSON ${SHADER_JSONS})
            # Reset the files list from any last iterations
            set(SHADER_FILES "")

            # Read the JSON file
            file(READ ${SHADER_JSON} JSON_STRING)
            string(JSON SHADER_COUNT LENGTH ${JSON_STRING} shaders)
            MATH(EXPR SHADER_COUNT "${SHADER_COUNT}-1") # foreach RANGE is inclusive

            # Get a list of all referenced source file paths
            cmake_path(GET SHADER_JSON PARENT_PATH SHADER_JSON_FOLDER)
            foreach(IDX RANGE ${SHADER_COUNT})
                string(JSON SOURCE_NAME GET ${JSON_STRING} shaders ${IDX} source)
                list(APPEND SHADER_FILES "${SHADER_JSON_FOLDER}/${SOURCE_NAME}")
            endforeach()
            message(STATUS ${SHADER_FILES})

            # Generate a hash of the full JSON path to uniquely identify it and its target.
            string(SHA1 JSON_PATH_HASH ${SHADER_JSON})
            set(SHADER_JSON_TARGET "build_shader_${JSON_PATH_HASH}")
            set(SHADER_TIMESTAMP_NAME "${JSON_PATH_HASH}.timestamp")

            add_custom_command(
                OUTPUT ${SHADER_TIMESTAMP_NAME}
                COMMAND $<TARGET_FILE:shaderprocessor> ${SHADER_JSON}
                COMMAND ${CMAKE_COMMAND} -E touch ${SHADER_TIMESTAMP_NAME}
                DEPENDS ${SHADER_FILES} ${SHADER_JSON} shaderprocessor::shaderprocessor
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                VERBATIM
                COMMENT "Processing ${SHADER_JSON}" # In 3.26 this could use generator expressions
            )
            add_custom_target(${SHADER_JSON_TARGET} DEPENDS ${SHADER_TIMESTAMP_NAME})
            add_dependencies(${SHADER_JSON_TARGET} shaderprocessor::shaderprocessor)
            add_dependencies(${TARGET_DEPENDENCY} ${SHADER_JSON_TARGET})
        endforeach()
        add_dependencies(${TARGET_DEPENDENCY} shaderprocessor)
    else()
        # Single-target fallback mechanism
        file(GLOB_RECURSE SHADER_FILES "${SHADER_DIRECTORY}/*")

        # Create a target that depends on the shaders and shader jsons which has to be built before
        # the game target, in which we execute the shader processor. By creating a build_shaders.timestamp
        # file, we can track the time of the last build to avoid unnecessary recompilations of the shaders.
        # We also depend on the shaderprocessor itself so that when it changes we rebuild all shaders.
        add_custom_command(
            OUTPUT build_shaders.timestamp
            COMMAND $<TARGET_FILE:shaderprocessor> ${SHADER_JSONS}
            COMMAND ${CMAKE_COMMAND} -E touch build_shaders.timestamp
            DEPENDS ${SHADER_FILES} ${SHADER_JSONS} shaderprocessor
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            VERBATIM
        )
        add_custom_target(build_shaders DEPENDS build_shaders.timestamp)
        add_dependencies(build_shaders shaderprocessor)
        add_dependencies(${TARGET_DEPENDENCY} shaderprocessor build_shaders)
    endif()
endmacro()
