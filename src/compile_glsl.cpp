#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <glslang/Public/ShaderLang.h>

#include <shaders/compile.hpp>
#include <shaders/glslang_resource.hpp>
#include <shaders/shader_constants.hpp>

namespace fs = std::filesystem;

class DefaultFileIncluder : public glslang::TShader::Includer {
	fs::path sourcePath;

public:
	explicit DefaultFileIncluder(fs::path sourcePath) : sourcePath(std::move(sourcePath)) {};

	~DefaultFileIncluder() override = default;

	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override {
		// We're treating system and local include paths as identical.
		// Though note that when includeLocal returns nullptr, includeSystem is called.
		return includeLocal(headerName, includerName, inclusionDepth);
	}

	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override {
		// TODO: Respect the relative directory of included files.
		auto fullPath = sourcePath / fs::path { headerName };

		std::ifstream file(fullPath, std::ios_base::binary | std::ios_base::ate);

		auto length = file.tellg();
		char* content = new char[length];
		file.seekg(0, std::ifstream::beg);
		file.read(content, length);
		return new IncludeResult(fullPath.string(), content, length, content);
	}

	void releaseInclude(IncludeResult* result) override {
		delete[] static_cast<char*>(result->userData);
		delete result;
	}
};

constexpr EShLanguage getGlslangStage(shaders::ShaderStage inputStage) {
	using namespace ::shaders;
	assert(std::popcount(static_cast<uint16_t>(inputStage)) == 1);

	switch (inputStage) {
		case ShaderStage::Vertex:
			return EShLangVertex;
		case ShaderStage::Fragment:
			return EShLangFragment;
		case ShaderStage::Geometry:
			return EShLangGeometry;
		case ShaderStage::Compute:
			return EShLangCompute;
		case ShaderStage::Mesh:
			return EShLangMesh;
		case ShaderStage::Task:
			return EShLangTask;
		case ShaderStage::RayGen:
			return EShLangRayGen;
		case ShaderStage::ClosestHit:
			return EShLangClosestHit;
		case ShaderStage::Miss:
			return EShLangMiss;
		case ShaderStage::AnyHit:
			return EShLangAnyHit;
		case ShaderStage::Intersect:
			return EShLangIntersect;
		case ShaderStage::Callable:
			return EShLangCallable;
		default:
			// TODO: Should we just depend on fmt?
			throw std::runtime_error(
				std::string { "[glslang] Unrecognized shader stage type: {}" } + std::to_string(static_cast<std::underlying_type_t<ShaderStage>>(inputStage)));
	}
}

constexpr glslang::EShTargetLanguageVersion getGlslangSpvVersion(shaders::SPVVersion spvVersion) {
	using namespace ::shaders;
	switch (spvVersion) {
		case SPVVersion::SPV_1_3:
			return glslang::EShTargetSpv_1_3;
		case SPVVersion::SPV_1_4:
			return glslang::EShTargetSpv_1_4;
		case SPVVersion::SPV_1_5:
			return glslang::EShTargetSpv_1_5;
		case SPVVersion::SPV_1_6:
			return glslang::EShTargetSpv_1_6;
		default:
			throw std::runtime_error(
				std::string { "[glslang] Unrecognized SPIR-V version: {}" } + std::to_string(static_cast<std::underlying_type_t<SPVVersion>>(spvVersion)));
	}
}

template <typename T>
// clang-format off
requires requires(T t) {
	{ t.getInfoLog() } -> std::same_as<const char*>;
	{ t.getInfoDebugLog() } -> std::same_as<const char*>;
}
// clang-format on
void printGlslangError(const std::string& filePath, T* shader) {
	std::string line;
	{
		std::istringstream ss(shader->getInfoLog());
		while (std::getline(ss, line)) {
			std::cerr << ">> [glslang] " << filePath << ": " << line << std::endl;
		}
	}

	{
		std::istringstream ss(shader->getInfoDebugLog());
		while (std::getline(ss, line)) {
			std::cerr << ">> [glslang] " << filePath << ": " << line << std::endl;
		}
	}
}

std::vector<std::uint32_t> shaders::compileGlsl(const shaders::ShaderJsonDesc& shaderStage) {
	// glslang only allows compiling a single shader called "main"
	assert(shaderStage.entryPoints.size() == 1);
	assert(shaderStage.entryPoints.front().name == "main");
	const auto& entryPoint = shaderStage.entryPoints.front();

	// Make this configurable in the future.
	constexpr auto spvVersion = shaders::SPVVersion::SPV_1_3;
	constexpr auto glslVersion = 460U;
	constexpr auto glslProfile = ENoProfile;
	constexpr auto messages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgEnhanced);
	auto stage = getGlslangStage(entryPoint.stage);

	// Read the file as a string.
	auto shaderSource = shaderStage.source.string();
	std::string glsl = readFileAsString(shaderStage.source);
	const auto* sourcePointer = glsl.data();

	auto shader = std::make_unique<glslang::TShader>(stage);
	shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, glslVersion);
	shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
	shader->setEnvTarget(glslang::EshTargetSpv, getGlslangSpvVersion(spvVersion));
	shader->setStrings(&sourcePointer, 1);

	std::string preprocessedGLSL;
	{
		DefaultFileIncluder includer(shaderStage.source.parent_path());
		if (!shader->preprocess(&shaders::DefaultTBuiltInResource, glslVersion, glslProfile, true, false, messages, &preprocessedGLSL,
		                        includer)) {
			printGlslangError(shaderSource, shader.get());
			shader.reset();
			return {};
		}
	}

	sourcePointer = preprocessedGLSL.data();
	shader->setStrings(&sourcePointer, 1);
	if (!shader->parse(&shaders::DefaultTBuiltInResource, glslVersion, glslProfile, true, false, messages)) {
		printGlslangError(shaderSource, shader.get());
		shader.reset();
		return {};
	}

	auto program = std::make_unique<glslang::TProgram>();
	program->addShader(shader.get());

	if (!program->link(messages)) {
		printGlslangError(shaderSource, program.get());
		program.reset();
		shader.reset();
		return {};
	}

	std::vector<std::uint32_t> spirv;
	spv::SpvBuildLogger spvBuildLogger;
	{
		glslang::SpvOptions spvOptions;
		const auto* intermediate = program->getIntermediate(stage);
		glslang::GlslangToSpv(*intermediate, spirv, &spvBuildLogger, &spvOptions);
	}

	{
		auto spvMessages = spvBuildLogger.getAllMessages();
		if (!spvMessages.empty()) {
			std::string line;
			std::istringstream ss(spvMessages);
			while (std::getline(ss, line)) {
				std::cerr << ">> [glslang] " << shaderSource << ": " << line << std::endl;
			}
		}
	}

	return spirv;
}
