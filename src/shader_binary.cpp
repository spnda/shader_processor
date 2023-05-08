#include <cstring>
#include <fstream>
#include <iostream>
#include <span>

#include <shaders/shader_binary.hpp>

namespace fs = std::filesystem;
namespace ks = ::shaders;

std::span<const std::string_view> shaders::ShaderLibrary::getShaderNames() const {
	return shaderNames;
}

const shaders::ShaderBinary* shaders::ShaderLibrary::getShaderBinaryByName(std::string_view shaderName) {
	auto it = std::find_if(binaries.begin(), binaries.end(), [&shaderName](ShaderBinary& binary) {
		return binary.shaderName == shaderName;
	});

	if (it == binaries.end()) {
		return nullptr;
	}
	return &(*it);
}

const shaders::ShaderBinary* shaders::ShaderLibrary::getShaderBinaryByStage(shaders::ShaderStage stage) {
	auto it = std::find_if(binaries.begin(), binaries.end(), [&stage](ShaderBinary& binary) {
		return binary.stage == stage;
	});

	if (it == binaries.end()) {
		return nullptr;
	}
	return &(*it);
}

std::vector<std::byte> shaders::buildShaderLibrary(std::vector<ShaderInput>&& inputs) {
	// We're not going to profile the shader_preprocessor.exe, so we'll not mark this as a zone.
	auto inputCount = static_cast<std::uint16_t>(inputs.size()); // This will also limit it.

	std::vector<std::byte> output;

	size_t totalShaderByteSize = 0;
	for (const auto& input : inputs) {
		totalShaderByteSize += input.shaderBytes.size();
		totalShaderByteSize += input.shaderName.size() + 1;
		totalShaderByteSize += input.name.size() + 1;
	}

	// Reserve enough space for the whole file.
	// clang-format off
	output.resize(sizeof(ShaderFileHeader) +
	              sizeof(ShaderDescription) * inputCount +
	              totalShaderByteSize);
	// clang-format on

	auto write = [output = output.data()](const void* data, std::size_t size) mutable {
		std::memcpy(output, data, size);
		output += size;
	};

	// Write the file header.
	{
		ShaderFileHeader header = {
			.magic = headerMagic,
			.shaderCount = inputCount,
		};
		write(&header, sizeof header);
	}

	// Write shader description headers. We also calculate the byte offsets for each component.
	auto data_offset = sizeof(ShaderFileHeader) + sizeof(ShaderDescription) * inputCount;
	for (const auto& input : inputs) {
		ShaderDescription description = {
			.byteOffset = data_offset + input.shaderName.size() + input.name.size(),
			.byteSize = input.shaderBytes.size(),
			.nameByteOffset = data_offset,
			.shaderNameByteOffset = data_offset + input.name.size(),
			.stage = input.stage,
			.lang = input.lang,
		};
		write(&description, sizeof description);
		data_offset = description.byteOffset + description.byteSize;
	}

	// Now that we've written all the headers, we begin writing all the data.
	for (const auto& input : inputs) {
		write(input.name.data(), input.name.size());
		write(input.shaderName.data(), input.shaderName.size());
		write(input.shaderBytes.data(), input.shaderBytes.size());
	}

	return output;
}

shaders::ShaderLibrary shaders::readShaderLibraryFromFile(const fs::path& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	auto length = file.tellg();
	file.seekg(0, std::ifstream::beg);

	if (length <= 0 || file.fail()) {
		std::cerr << "Failed to open shader binary file: " << path << std::endl;
		return {};
	}

	if (length < sizeof(ShaderFileHeader)) {
		std::cerr << "Shader binary file too small: " << length << " bytes" << std::endl;
		return {};
	}

	// This assumes that we read everything in linear order. So this needs changing should the
	// write function ever change.
	auto read = [&file](void* data, std::size_t size) mutable [[gnu::always_inline]] { file.read(reinterpret_cast<char*>(data), size); };

	ShaderFileHeader header = {};
	read(&header, sizeof header);
	if (header.magic != headerMagic) {
		std::string_view magic = { reinterpret_cast<char*>(&header.magic), 4 };
		std::string_view correctMagic = { reinterpret_cast<const char*>(&headerMagic), 4 };
		std::cerr << "Invalid magic header on shader binary file: " << magic << " != " << correctMagic << std::endl;
		return {};
	}

	ShaderLibrary library;
	library.shaderNames.resize(header.shaderCount);
	library.binaries.resize(header.shaderCount);

	std::vector<ShaderDescription> descriptions(header.shaderCount);

	// We first read all the shader descriptions
	for (auto i = 0U; i < header.shaderCount; ++i) {
		ShaderDescription description = {};
		read(&description, sizeof description);
		descriptions[i] = description;
	}

	// Then read all names and binaries.
	for (auto i = 0U; i < header.shaderCount; ++i) {
		auto& binary = library.binaries[i];
		auto& desc = descriptions[i];

		binary.stage = desc.stage;
		binary.lang = desc.lang;

		// We read the name first and determine its size by looking where the shader bytes begin.
		binary.name.resize(desc.shaderNameByteOffset - desc.nameByteOffset);
		read(binary.name.data(), binary.name.size());

		binary.shaderName.resize(desc.byteOffset - desc.shaderNameByteOffset);
		read(binary.shaderName.data(), binary.shaderName.size());

		binary.bytes.resize(desc.byteSize);
		read(binary.bytes.data(), binary.bytes.size());

		library.shaderNames[i] = binary.shaderName;
	}

	return library;
}
