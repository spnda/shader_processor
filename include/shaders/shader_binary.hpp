#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace shaders {
	enum class ShaderStage : uint16_t;
	enum class ShaderLang : uint8_t;

	[[nodiscard]] constexpr uint32_t fourCharacterCode(char char1, char char2, char char3, char char4) {
		return static_cast<uint32_t>(char1) | (static_cast<uint32_t>(char2) << 8) | (static_cast<uint32_t>(char3) << 16)
		       | (static_cast<uint32_t>(char4) << 24);
	}

	inline constinit const auto headerMagic = fourCharacterCode('!', 'K', 'R', 'S');

	struct ShaderFileHeader {
		uint32_t magic;
		// This specifies the count of uint64_t integers directly afterwards that represent
		// the byte offset into the file excluding this header for that specific shader.
		uint16_t shaderCount;
	};

	struct alignas(uint64_t) ShaderDescription {
		uint64_t byteOffset;     // The byte offset for the shader binary.
		uint64_t byteSize;       // The size of the binary.
		uint64_t nameByteOffset; // The byte offset for the name string.
		ShaderStage stage;       // 16 bits.
		ShaderLang lang;         // 8 bits. This should only be SPIR-V or AIR.
	};

	struct ShaderBinary {
		ShaderDescription desc;
		std::string name;
		std::vector<std::byte> bytes;
	};

	struct ShaderInput {
		std::vector<std::byte> shaderBytes;
		std::string name;
		ShaderStage stage;
		ShaderLang lang;
	};

	class ShaderLibrary;

	[[nodiscard]] std::vector<std::byte> buildShaderLibrary(std::vector<ShaderInput>&& inputs);
	[[nodiscard]] ShaderLibrary readShaderLibraryFromFile(const std::filesystem::path& path);

	class ShaderLibrary {
		friend ShaderLibrary readShaderLibraryFromFile(const std::filesystem::path& path);

		std::string name;
		// Each element in binaries shall make up binary for a single shader function. Each entry
		// point name will be tracked in the functionNames vector.
		std::vector<std::string_view> functionNames;
		std::vector<ShaderBinary> binaries;

	public:
		[[nodiscard]] std::span<const std::string_view> getFunctionNames() const;
		[[nodiscard]] const ShaderBinary* getBinaryByName(std::string_view name);
		[[nodiscard]] const ShaderBinary* getBinaryByStage(ShaderStage stage);
	};
} // namespace shaders
