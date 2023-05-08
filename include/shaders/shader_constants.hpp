#pragma once

#include <cstdint>
#include <type_traits>

namespace shaders {
	// clang-format off
	enum class ShaderLang : uint8_t {
		GLSL    = 1 << 0,
		HLSL    = 1 << 1,
		SLANG   = 1 << 2,
		SPIRV   = 1 << 3,
		MSL     = 1 << 4,
		AIR     = 1 << 5, // Apple IR
	};

	constexpr ShaderLang operator&(ShaderLang lhs, ShaderLang rhs) {
		using EnumType = std::underlying_type_t<ShaderLang>;
		return static_cast<ShaderLang>(static_cast<EnumType>(lhs) & static_cast<EnumType>(rhs));
	}
	// clang-format on

	// We do 1.3 to 1.6.
	enum class SPVVersion : uint8_t {
		SPV_1_3, // Vulkan 1.1
		SPV_1_4, // Requires KHR_spirv_1_4 (1.1) or Vulkan 1.2
		SPV_1_5, // Requires Vulkan 1.2
		SPV_1_6, // Requires Vulkan 1.3
	};

	// clang-format off
	enum class ShaderStage : uint16_t {
		None        = 0,
		Vertex      = 1 <<  0,
		Fragment    = 1 <<  1,
		Geometry    = 1 <<  2,
		Compute     = 1 <<  3,
		Mesh        = 1 <<  4,
		Task        = 1 <<  5,
		RayGen      = 1 <<  6,
		ClosestHit  = 1 <<  7,
		Miss        = 1 <<  8,
		AnyHit      = 1 <<  9,
		Intersect   = 1 << 10,
		Callable    = 1 << 11,
	};
	// clang-format on
} // namespace shaders
