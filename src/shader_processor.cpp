#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <span>

#ifdef WITH_SLANG_SHADERS
#include <slang.h>
#endif

#ifdef WITH_GLSLANG_SHADERS
#include <glslang/Public/ShaderLang.h>
#endif

#ifdef WITH_SPIRV_CROSS
#include <spirv_cross_c.h>
#endif

#include <shaders/compile.hpp>
#include <shaders/shader_binary.hpp>
#include <shaders/shader_json.hpp>
#include <shaders/shader_constants.hpp>

namespace fs = std::filesystem;

std::string shaders::readFileAsString(const fs::path& path) {
	std::ifstream fileInput(path);
	std::stringstream fileContents;
	fileContents << fileInput.rdbuf();
	return fileContents.str();
}

std::vector<std::byte> shaders::readFileAsBytes(const fs::path& path) {
	std::ifstream fileInput(path, std::ios::binary);
	fileInput.ignore(std::numeric_limits<std::streamsize>::max());
	auto length = fileInput.gcount();
	fileInput.clear();
	fileInput.seekg(0, std::ifstream::beg);

	std::vector<std::byte> bytes(length);
	fileInput.seekg(0);
	fileInput.read(reinterpret_cast<char*>(bytes.data()), length);
	return bytes;
}

std::int32_t processJson(fs::path path) noexcept {
	shaders::ShaderJson json;
	auto error = shaders::parseJson(path, json);
	if (error != 0) {
		return error;
	}

	if (json.descriptions.empty()) {
		std::cerr << "No shaders specified in file: " << path << std::endl;
		return -1;
	}

	auto outputFolder = fs::current_path() / "shaders";
	std::vector<shaders::ShaderInput> shaderInputs;
	shaderInputs.reserve(json.descriptions.size());
	for (auto& stage : json.descriptions) {
		std::cout << ">> " << stage.source.filename() << std::endl;
		const auto& frontEntry = stage.entryPoints.front();

		// If the target is the same as the input, we'll just copy the data.
		if (stage.target == stage.lang) {
			shaderInputs.emplace_back(shaders::ShaderInput {
				.shaderBytes = shaders::readFileAsBytes(stage.source),
				.name = frontEntry.name,
				.stage = frontEntry.stage,
				.lang = stage.target,
			});
		}

		switch (stage.lang) {
			case shaders::ShaderLang::GLSL: {
#ifdef WITH_GLSLANG_SHADERS
				auto spirv = compileGlsl(stage);
				if (spirv.empty()) {
					std::cerr << "Failed to compile GLSL." << std::endl;
					continue;
				}

				// We need to copy here. It breaks the data if we use vector constructors.
				std::vector<std::byte> spirvBytes(spirv.size() * sizeof(std::uint32_t));
				std::memcpy(spirvBytes.data(), spirv.data(), spirv.size() * sizeof(std::uint32_t));

				if ((stage.target & shaders::ShaderLang::SPIRV) == stage.target) {
					shaderInputs.emplace_back(shaders::ShaderInput {
#ifdef __APPLE__
						.shaderBytes = spirvBytes,
#else
						.shaderBytes = std::move(spirvBytes),
#endif
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::SPIRV,
					});
				}
#ifdef __APPLE__
				if (ku::hasBit(stage.target, shaders::ShaderLang::MSL)) {
					auto msl = shaders::generateMslFromSpv(spirvBytes);

					std::vector<std::byte> bytes(msl.size());
					std::memcpy(bytes.data(), msl.data(), msl.size());

					shaderInputs.emplace_back(shaders::ShaderInput {
						.shaderBytes = std::move(bytes),
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::MSL,
					});
				}
#endif
#endif
				break;
			}
			case shaders::ShaderLang::SLANG: {
#ifdef WITH_SLANG_SHADERS
				auto spirv = ::shaders::compileSlang(stage);
				for (auto it = spirv.begin(); it != spirv.end(); ++it) {
					auto index = std::distance(spirv.begin(), it);

					if (it->empty()) {
						kl::err("Failed to compile SLANG: {}", stage.entryPoints[index].name);
						continue;
					}

					std::vector<std::byte> spirvBytes(it->size() * sizeof(std::uint32_t));
					std::memcpy(spirvBytes.data(), it->data(), it->size() * sizeof(std::uint32_t));
					shaderInputs.emplace_back(shaders::ShaderInput {
						.shaderBytes = std::move(spirvBytes),
						.name = stage.entryPoints[index].name,
						.stage = stage.entryPoints[index].stage,
						.lang = shaders::ShaderLang::SPIRV,
					});

#ifdef __APPLE__
					if (ku::hasBit(stage.target, shaders::ShaderLang::MSL)) {
						auto msl = shaders::generateMslFromSpv(spirvBytes);
						shaderInputs.emplace_back(shaders::ShaderInput {
							.shaderBytes = std::vector { msl.begin(), msl.end() },
							.name = frontEntry.name,
							.stage = frontEntry.stage,
							.lang = shaders::ShaderLang::MSL,
						});
					}
#endif
				}
#endif
				break;
			}
			case shaders::ShaderLang::MSL: {
#ifdef __APPLE__
				if (ku::hasBit(stage.target, shaders::ShaderLang::AIR)) {
					auto air = shaders::compileMsl(stage);
					shaderInputs.emplace_back(shaders::ShaderInput {
						.shaderBytes = std::move(air),
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::AIR,
					});
				}

				if (ku::hasBit(stage.target, shaders::ShaderLang::MSL)) {
					auto msl = shaders::readFileAsBytes(stage.source);
					shaderInputs.emplace_back(shaders::ShaderInput {
						.shaderBytes = std::move(msl),
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::MSL,
					});
				}
#endif
				break;
			}
			case shaders::ShaderLang::SPIRV: {
				auto shaderBytes = shaders::readFileAsBytes(stage.source);
				if ((stage.target & shaders::ShaderLang::SPIRV) == stage.target) {
					shaderInputs.emplace_back(shaders::ShaderInput {
#ifdef __APPLE__
						.shaderBytes = shaderBytes,
#else
						.shaderBytes = std::move(shaderBytes),
#endif
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::SPIRV,
					});
				}
#ifdef __APPLE__
				if (ku::hasBit(stage.target, shaders::ShaderLang::MSL)) {
					auto msl = shaders::generateMslFromSpv(shaderBytes);

					std::vector<std::byte> bytes(msl.size());
					std::memcpy(bytes.data(), msl.data(), msl.size());

					shaderInputs.emplace_back(shaders::ShaderInput {
						.shaderBytes = std::move(bytes),
						.name = frontEntry.name,
						.stage = frontEntry.stage,
						.lang = shaders::ShaderLang::MSL,
					});
				}
#endif
				break;
			}
			default: {
				std::cerr << ">> Did not find a method to compile shader stage from source: " << stage.source.filename() << std::endl;
			}
		}
	}

	if (shaderInputs.empty()) {
		std::cerr << "All shaders failed to compile. Cannot build binary \"" << json.name << "\"." << std::endl;
		return 0; // We'll try processing other JSONs.
	}

	auto binaryBytes = shaders::buildShaderLibrary(std::move(shaderInputs));
	std::ofstream out(outputFolder / (json.name + ".shader"), std::ios::binary | std::ios::out);
	out.write(reinterpret_cast<const char*>(binaryBytes.data()), static_cast<int64_t>(binaryBytes.size()));

	return 0;
}

int main(int argc, char* argv[]) {
	if (argc <= 1) { // Why is argc signed anyway?
		std::cerr << "No json path specified. " << std::endl;
		return -1;
	}

	{
		auto outputFolder = fs::current_path() / "shaders";
		if (!fs::exists(outputFolder)) {
			fs::create_directory(outputFolder);
		}
	}

#ifdef WITH_GLSLANG_SHADERS
	glslang::InitializeProcess();
#endif

#ifdef WITH_SLANG_SHADERS
	shaders::slangSession = spCreateSession(); // Not fully threadsafe.
#endif

#ifdef WITH_SPIRV_CROSS
	spvc_context_create(&shaders::spvcContext);
#endif

	std::span<char*> args = { std::next(argv), static_cast<size_t>(argc - 1) };
	for (auto& arg : args) {
		std::cout << "Parsing " << arg << std::endl;
		auto ret = processJson(fs::path { arg });
		if (ret != 0) {
			return ret;
		}
	}

#ifdef WITH_SPIRV_CROSS
	spvc_context_release_allocations(shaders::spvcContext);
	spvc_context_destroy(shaders::spvcContext);
#endif

#ifdef WITH_SLANG_SHADERS
	spDestroySession(shaders::slangSession);
#endif

#ifdef WITH_GLSLANG_SHADERS
	glslang::FinalizeProcess();
#endif

	return 0;
}
