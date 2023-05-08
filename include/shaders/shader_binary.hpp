#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace shaders {
	enum class ShaderStage : std::uint16_t;
	enum class ShaderLang : std::uint8_t;

	[[nodiscard]] constexpr std::uint32_t fourCharacterCode(char char1, char char2, char char3, char char4) {
		return static_cast<std::uint32_t>(char1) | (static_cast<std::uint32_t>(char2) << 8) | (static_cast<std::uint32_t>(char3) << 16)
		       | (static_cast<std::uint32_t>(char4) << 24);
	}

	inline constinit const auto headerMagic = fourCharacterCode('!', 'S', 'B', 'F');

	struct ShaderFileHeader {
		std::uint32_t magic;
		// This specifies the count of uint64_t integers directly afterward that represent
		// the byte offset into the file excluding this header for that specific shader.
		std::uint16_t shaderCount;
	};

	struct alignas(std::uint64_t) ShaderDescription {
		std::uint64_t byteOffset;           // The byte offset for the shader binary.
		std::uint64_t byteSize;             // The size of the binary.
		std::uint64_t nameByteOffset;       // The byte offset for the entry point name string.
		std::uint64_t shaderNameByteOffset; // The byte offset for the shader name string.
		ShaderStage stage;                  // 16 bits.
		ShaderLang lang;                    // 8 bits. This should only be SPIR-V or AIR.
	};

	struct ShaderBinary {
		ShaderStage stage;
		ShaderLang lang;
		std::string name;
		std::string shaderName;
		std::vector<std::byte> bytes;
	};

	struct ShaderInput {
		std::vector<std::byte> shaderBytes;
		// The name of the shader that this input comes from.
		std::string shaderName;
		// The name of the entry point.
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
		std::vector<std::string_view> shaderNames;
		std::vector<ShaderBinary> binaries;

	public:
		[[nodiscard]] std::span<const std::string_view> getShaderNames() const;
		[[nodiscard]] const ShaderBinary* getShaderBinaryByName(std::string_view name);
		// This will return the first shader in the binary that has the given shader stage, regardless
		// of whether other shaders with the same stage are available.
		[[nodiscard]] const ShaderBinary* getShaderBinaryByStage(ShaderStage stage);
	};
} // namespace shaders
