#include <cstdint>
#include <iostream>

#ifndef SIMDJSON_EXCEPTIONS
#define SIMDJSON_EXCEPTIONS 1
#endif

#include <simdjson.h>
#include <magic_enum.hpp>

#include <shaders/shader_json.hpp>

namespace fs = std::filesystem;

template <typename T>
T getEnumForJsonString(simdjson::simdjson_result<simdjson::dom::element> string, std::string_view name) {
	if (!string.is_string() || string.error() != 0) {
		std::cerr << "Missing string value: " << name << std::endl;
		return static_cast<T>(0);
	}
	auto str = string.get_string().value();
	auto ret = magic_enum::enum_cast<T>(str);
	if (!ret.has_value()) {
		// kl::err("Invalid enum value for field \"{}\": {}", name, str);
		return static_cast<T>(0);
	}
	return ret.value();
}

std::int32_t shaders::parseJson(fs::path& path, shaders::ShaderJson& shader) {
	if (!fs::exists(path)) {
		std::cerr << "JSON file does not exist: " << path << std::endl;
		return -1;
	}

	simdjson::dom::parser parser;
	auto doc = parser.load(path.string());

	std::string_view nameView;
	{
		auto error = doc["name"].get_string().get(nameView);
		if (error != 0) {
			std::cerr << "Failed to get name from JSON: " << simdjson::error_message(error) << std::endl;
			return -1;
		}
	}
	shader.name = nameView;

	simdjson::dom::array shadersArray;
	{
		auto error = doc["shaders"].get_array().get(shadersArray);
		if (error != 0) {
			std::cerr << "Failed to get stages array from JSON: " << simdjson::error_message(error) << std::endl;
			return -1;
		}
	}

	// Reserve incase we have a lot of stages.
	shader.descriptions.reserve(shadersArray.size());

	auto folder = path.parent_path();
	for (auto element : shadersArray) {
		// These two are required.
		auto source = element["source"];
		auto target = element["target"];
		auto lang = element["lang"];

		if ((source.error() != 0 || target.error() != 0 || lang.error() != 0)
		    && (!source.is_string() || !target.is_string() || !lang.is_string())) {
			std::cerr << "Missing or invalid stage, source, target, or lang fields for stage. Skipping stage." << std::endl;
			continue;
		}

		auto entryPoints = element["entryPoints"];
		std::vector<ShaderEntryPoint> entryPointObjects;
		if (entryPoints.is_array()) {
			auto entryPointArray = entryPoints.get_array().value();
			for (auto entryPoint : entryPointArray) {
				if (entryPoint.is_object()) {
					auto name = entryPoint["name"];
					auto stage = entryPoint["stage"];

					if ((name.error() != 0 || stage.error() != 0) && (!name.is_string() || !stage.is_string())) {
						std::cerr << "Malformed entryPoints." << std::endl;
						continue;
					}

					auto stageString = std::string { stage.get_string().value() };
					if (!stageString.empty()) {
						// Our enums are capitalised so the string also needs to be capitalised.
						// Not sure why std::toupper and std::tolower use ints.
						stageString[0] = static_cast<char>(std::toupper(stageString[0]));
					}

					auto shaderStage = magic_enum::enum_cast<ShaderStage>(stageString);
					if (!shaderStage.has_value()) {
						std::cerr << "Invalid shader stage: " << stageString << std::endl;
						continue;
					}

					auto nameString = name.get_string().value();
					entryPointObjects.emplace_back(shaders::ShaderEntryPoint {
						.stage = shaderStage.value(),
						.name = std::string { nameString },
					});
				} else {
					std::cerr << "Malformed entryPoints. " << std::endl;
				}
			}
		} else {
			std::cerr << "No entry points specified for stage: " << source.get_string().value() << std::endl;
			continue;
		}

		auto stageTarget = getEnumForJsonString<ShaderLang>(target, "target");
		auto stageLang = getEnumForJsonString<ShaderLang>(lang, "lang");

		if (std::popcount(static_cast<std::underlying_type_t<decltype(stageTarget)>>(stageTarget)) > 1) {
			std::cerr << "Only one target output is currently supported." << std::endl;
		}

		auto sourcePath = fs::path { source.get_string().value() };
		shader.descriptions.emplace_back(ShaderJsonDesc {
			.source = folder / sourcePath,
			.lang = stageLang,
			.targets = stageTarget,
			.entryPoints = std::move(entryPointObjects),
		});
	}

	return 0;
}
