#import <vector>

#import <Foundation/Foundation.h>

#import <shaders/compile.hpp>

std::vector<std::byte> shaders::compileMsl(const shaders::ShaderJsonStage& shaderStage) {
	@autoreleasepool {
		NSString* path = nil;
		{
			auto source = shaderStage.source.string();
			path = [NSString stringWithUTF8String:source.c_str()];
		}
		// TODO: Somehow place this into the CMake build directory?
		auto airOutput = [NSString stringWithFormat:@"%@%@", path, @".air"];
		auto libOutput = [NSString stringWithFormat:@"%@%@", path, @".metallib"];

		// The only way to compile MSL to AIR is by executing the "metal" command-line tool.
		// As we're invoking xcrun, this won't work when sandboxed.
		auto airTask = [[NSTask alloc] init];
		[airTask setExecutableURL:[NSURL fileURLWithPath:@"/usr/bin/xcrun"]];
		[airTask setArguments:@[ @"-sdk", @"macosx", @"metal", @"-frecord-sources", @"-c", path, @"-o", airOutput ]];
		[airTask launch];
		[airTask waitUntilExit];

		auto libTask = [[NSTask alloc] init];
		[libTask setExecutableURL:[NSURL fileURLWithPath:@"/usr/bin/xcrun"]];
		[libTask setArguments:@[ @"-sdk", @"macosx", @"metallib", airOutput, @"-o", libOutput ]];
		[libTask launch];
		[libTask waitUntilExit];

		// Read the compiled AIR file as bytes.
		auto ret = readFileAsBytes(fs::path{[libOutput UTF8String]});

		// Delete the AIR file.
		NSError* error = nil;
		auto* manager = [NSFileManager defaultManager];
		[manager removeItemAtPath:airOutput error:&error];
		[manager removeItemAtPath:libOutput error:&error];
		return ret;
	}
}
