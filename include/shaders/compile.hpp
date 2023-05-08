#pragma once

#include <span>
#include <vector>

#include <shaders/shader_json.hpp>

#ifdef WITH_SPIRV_CROSS
typedef struct spvc_context_s* spvc_context;
#endif

#ifdef WITH_SLANG_SHADERS
namespace slang {
	struct IGlobalSession;
} // namespace slang
typedef slang::IGlobalSession SlangSession;
#endif

// This header includes the function declarations for all compile funcs for various compilers.
namespace shaders {
#ifdef WITH_SPIRV_CROSS
	inline spvc_context spvcContext = nullptr;
#endif

#ifdef WITH_SLANG_SHADERS
	inline SlangSession* slangSession = nullptr;
#endif

	std::string readFileAsString(const std::filesystem::path& path);
	std::vector<std::byte> readFileAsBytes(const std::filesystem::path& path);

#ifdef WITH_SPIRV_CROSS
	std::string generateMslFromSpv(std::span<std::byte> bytes);
	std::string generateMslFromSpv(const ShaderJsonDesc& shaderJsonStage);
#endif

#ifdef WITH_GLSLANG_SHADERS
	std::vector<uint32_t> compileGlsl(const ShaderJsonDesc& shaderStage);
#endif

#ifdef WITH_SLANG_SHADERS
	std::vector<std::vector<uint32_t>> compileSlang(const ShaderJsonStage& shaderStage);
#endif

#ifdef __APPLE__
	std::vector<std::byte> compileMsl(const ShaderJsonStage& shaderStage);
#endif
} // namespace shaders
