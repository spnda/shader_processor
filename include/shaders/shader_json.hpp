#pragma once

#include <filesystem>

#include <shaders/shader_constants.hpp>

namespace shaders {
	struct ShaderEntryPoint {
		ShaderStage stage;
		std::string name;
	};

	struct ShaderJsonDesc {
		std::filesystem::path source;
		ShaderLang lang;
		ShaderLang target;
		std::vector<ShaderEntryPoint> entryPoints;
	};

	struct ShaderJson {
		std::string name;
		std::vector<ShaderJsonDesc> descriptions;
	};

	[[nodiscard]] std::int32_t parseJson(std::filesystem::path& path, ShaderJson& shader);
} // namespace shaders
