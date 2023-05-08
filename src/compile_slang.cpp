#include <fstream>

#include <slang.h>

#include <shaders/compile.hpp>

SlangStage getSlangStage(shaders::ShaderStage inputStage) {
	using namespace ks;
	VERIFY(std::popcount(static_cast<std::std::uint16_t>(inputStage)) == 1);

	switch (inputStage) {
		case ShaderStage::Vertex:
			return SLANG_STAGE_VERTEX;
		case ShaderStage::Fragment:
			return SLANG_STAGE_FRAGMENT;
		case ShaderStage::Geometry:
			return SLANG_STAGE_GEOMETRY;
		case ShaderStage::Compute:
			return SLANG_STAGE_COMPUTE;
		case ShaderStage::Mesh:
			return SLANG_STAGE_MESH;
		case ShaderStage::RayGen:
			return SLANG_STAGE_RAY_GENERATION;
		case ShaderStage::ClosestHit:
			return SLANG_STAGE_CLOSEST_HIT;
		case ShaderStage::Miss:
			return SLANG_STAGE_MISS;
		case ShaderStage::AnyHit:
			return SLANG_STAGE_ANY_HIT;
		case ShaderStage::Intersect:
			return SLANG_STAGE_INTERSECTION;
		case ShaderStage::Callable:
			return SLANG_STAGE_CALLABLE;
		default:
			kl::throwError("[slang] Unrecognized shader stage type: {}.", static_cast<std::uint16_t>(inputStage));
	}
}

std::vector<std::vector<std::uint32_t>> shaders::compileSlang(const ::shaders::ShaderJsonStage& shaderStage) {
	auto* session = static_cast<SlangSession*>(shaders::slangSession);

	constexpr SlangCompileTarget compileTarget = SLANG_SPIRV;
	constexpr SlangSourceLanguage source = SLANG_SOURCE_LANGUAGE_SLANG;

	auto* request = spCreateCompileRequest(session);
	auto target = spAddCodeGenTarget(request, compileTarget);

	// Setup some settings. We force the matrix layout to match GLSL.
	spAddSearchPath(request, shaderStage.source.parent_path().string().c_str());
	spSetDebugInfoLevel(request, SLANG_DEBUG_INFO_LEVEL_NONE);
	spSetOptimizationLevel(request, SLANG_OPTIMIZATION_LEVEL_HIGH);
	spSetMatrixLayoutMode(request, SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
	spSetTargetForceGLSLScalarBufferLayout(request, target, true);

	// Read the file as a string.
	std::string glsl;
	{
		std::ifstream fileInput(shaderStage.source);
		std::stringstream fileContents;
		fileContents << fileInput.rdbuf();
		glsl = fileContents.str();
	}

	auto filename = shaderStage.source.filename().string();
	auto tuIndex = spAddTranslationUnit(request, source, filename.c_str());
	spAddTranslationUnitSourceString(request, tuIndex, filename.c_str(), glsl.data());

	std::vector<std::int32_t> entryPoints;
	for (const auto& entry : shaderStage.entryPoints) {
		auto stage = getSlangStage(entry.stage);
		entryPoints.emplace_back(spAddEntryPoint(request, tuIndex, entry.name.c_str(), stage));
	}

	auto errors = spCompile(request);
	const auto* diagnostics = spGetDiagnosticOutput(request);

	if (errors != 0) {
		std::istringstream ss(diagnostics);
		std::string line;
		while (std::getline(ss, line)) {
			kl::err("[slang] {}: {}", shaderStage.source.filename().string(), line);
		}
		return {};
	}

	std::vector<std::vector<std::uint32_t>> results;
	results.reserve(entryPoints.size());
	for (auto& entryPoint : entryPoints) {
		// Get the shader output code for this entry point.
		size_t resultSize = 0;
		const auto* data = spGetEntryPointCode(request, entryPoint, &resultSize);
		VERIFY(resultSize > 0 && resultSize % 4 == 0); // SPIR-V requirements.

		// We have to reallocate the data as slang automatically deletes
		// the given data.
		std::vector<std::uint32_t> result(resultSize);
		std::memcpy(result.data(), data, resultSize);
		results.emplace_back(std::move(result));
	}

	spDestroyCompileRequest(request);

	return results;
}
