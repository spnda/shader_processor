#include <iostream>
#include <string>

// We're using the C API from the submodule, as the C++ API is not 100% stable
#include <spirv_cross_c.h>

#include <shaders/compile.hpp>

void printSpvcError(void* userData, const char* error) {
	std::cerr << "SPIRV-Cross: " << error << std::endl;
}

void checkSpvcReturn(spvc_result result, const std::string& message) {
	if (result != spvc_result::SPVC_SUCCESS) {
		std::cerr << "SPIRV-Cross: " << result << std::endl;
	}
}

std::string shaders::generateMslFromSpv(std::span<std::byte> spirv) {
	spvc_compiler compiler = nullptr;
	spvc_compiler_options options = nullptr;
	spvc_parsed_ir ir = nullptr;

	spvc_context_create(&spvcContext);

	spvc_context_set_error_callback(spvcContext, printSpvcError, nullptr);

	spvc_context_parse_spirv(spvcContext, reinterpret_cast<const std::uint32_t*>(spirv.data()), spirv.size_bytes(), &ir);
	spvc_context_create_compiler(spvcContext, SPVC_BACKEND_MSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);

	spvc_compiler_create_compiler_options(compiler, &options);
	spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION, SPVC_MAKE_MSL_VERSION(3, 0, 0));
	spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS, true);
	spvc_compiler_install_compiler_options(compiler, options);

	const char* tempString = nullptr;
	checkSpvcReturn(spvc_compiler_compile(compiler, &tempString), "Failed to compile: {}");

	if (tempString == nullptr) {
		std::cerr << "Failed to compile with SPIRV-Cross. " << std::endl;
		return {};
	}

	std::string msl(tempString);

	return msl;
}

std::string shaders::generateMslFromSpv(const ::shaders::ShaderJsonDesc& shaderJsonStage) {
	auto spirv = readFileAsBytes(shaderJsonStage.source);
	return generateMslFromSpv(spirv);
}
