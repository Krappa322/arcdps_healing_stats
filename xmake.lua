rule("protobuf")
	set_extensions(".proto")
	before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
		local targetfile = path.filename(sourcefile)
		local basedir = sourcefile:sub(1, - (#targetfile + 1))
		local basename = path.basename(sourcefile)
		local outputdir = target:autogendir()

		if not os.isdir(outputdir) then
			print("Creating output dir "..outputdir)
			os.mkdir(outputdir)
		end

		target:add("includedirs", outputdir)

		local grpc_cpp_plugin = os.iorunv("which grpc_cpp_plugin"):sub(1, -2)
		local protoc = "vcpkg_installed/x64-linux/x64-linux/tools/protobuf/protoc"

		batchcmds:show_progress(opt.progress, "${color.build.object}compiling.proto_source %s", sourcefile)
		batchcmds:vrunv(protoc, {"--cpp_out="..outputdir, "--grpc_out="..outputdir, "--plugin=protoc-gen-grpc="..grpc_cpp_plugin, "--proto_path="..basedir, targetfile})

		local lowest_mtime = nil
		for _, extension in pairs({".pb.cc", ".pb.h", ".grpc.pb.cc", ".grpc.pb.h"}) do
			file = path.join(outputdir, basename..extension)

			local mtime = os.mtime(file)
			if (lowest_mtime == nil) or (mtime < lowest_mtime) then
				lowest_mtime = mtime
			end
		end

		local depcache = target:dependfile(sourcefile.."_source")
		batchcmds:add_depfiles(sourcefile, protoc)
		batchcmds:set_depmtime(lowest_mtime)
		batchcmds:set_depcache(depcache)

		--batchcmds:show("depcache "..depcache.." mtime "..lowest_mtime)
	end)
	on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
		local targetfile = path.filename(sourcefile)
		local basedir = sourcefile:sub(1, - (#targetfile + 1))
		local basename = path.basename(sourcefile)
		local outputdir = target:autogendir()

		local lowest_mtime = nil
		for _, extension in pairs({".pb.cc", ".grpc.pb.cc"}) do
			file = path.join(outputdir, basename..extension)

			local objectfile = target:objectfile(file)
			table.insert(target:objectfiles(), objectfile)

			batchcmds:show_progress(opt.progress, "${color.build.object}compiling.proto_object %s", file)
			batchcmds:compile(file, objectfile)

			local mtime = os.mtime(objectfile)
			if (lowest_mtime == nil) or (mtime < lowest_mtime) then
				lowest_mtime = mtime
			end
		end

		local depcache = target:dependfile(sourcefile.."_object")

		batchcmds:add_depfiles(sourcefile)
		batchcmds:set_depmtime(lowest_mtime)
		batchcmds:set_depcache(depcache)

		--batchcmds:show("depcache "..depcache.." mtime "..lowest_mtime)
	end)

target("evtc_rpc_server")
	add_rules("protobuf")

	set_kind("binary")
	set_warnings("all")
	set_languages("c++17")
	set_toolset("cxx", "clang++")
	set_toolset("ld", "clang++")

	compilerflags = {}
	if is_mode("debug") then
		add_options("debug")
		add_defines("_DEBUG")

	elseif is_mode("asan") then
		set_optimize("none")
		add_defines("_DEBUG")
		table.insert(compilerflags, "-fsanitize=address")
		add_ldflags("-fsanitize=address")

	elseif is_mode("tsan") then
		set_optimize("none")
		add_defines("_DEBUG")
		table.insert(compilerflags, "-fsanitize=thread")
		add_ldflags("-fsanitize=thread")

	elseif is_mode("ubsan") then
		set_optimize("none")
		add_defines("_DEBUG")
		table.insert(compilerflags, "-fsanitize=undefined")
		table.insert(compilerflags, "-fno-sanitize=alignment")
		add_ldflags("-fsanitize=undefined")
		--add_ldflags("-fno-sanitize=alignment")

	elseif is_mode("release") then
		set_optimize("fastest")
		add_defines("NDEBUG")
		add_cxxflags("-flto")
		add_ldflags("-flto")
		add_ldflags("-fuse-linker-plugin")

	elseif is_mode("profiling") then
		set_optimize("fastest")
		add_defines("NDEBUG")
		add_cxxflags("-flto")
		add_ldflags("-flto")
		add_ldflags("-fuse-linker-plugin")
		add_cxxflags("-pg")
		add_ldflags("-pg")

	end

	add_defines("LINUX")

	add_syslinks("pthread")

	add_includedirs("arcdps_mock/arcdps-extension", "vcpkg_installed/x64-linux/x64-linux/include")
	add_linkdirs("vcpkg_installed/x64-linux/x64-linux/lib")

	-- Add everything absl as a group since they have circular dependencies
	absl_libs = {
		"failure_signal_handler",
		"flags_usage",
		"spinlock_wait",
		"malloc_internal",
		"cordz_functions",
		"strings_internal",
		"random_internal_randen_slow",
		"hash",
		"hashtablez_sampler",
		"random_seed_sequences",
		"demangle_internal",
		"bad_variant_access",
		"random_internal_randen_hwaes_impl",
		"flags_parse",
		"time_zone",
		"flags_marshalling",
		"random_internal_seed_material",
		"low_level_hash",
		"log_severity",
		"periodic_sampler",
		"raw_logging_internal",
		"cordz_sample_token",
		"civil_time",
		"graphcycles_internal",
		"leak_check",
		"symbolize",
		"examine_stack",
		"random_internal_pool_urbg",
		"random_internal_platform",
		"random_internal_distribution_test_util",
		"flags_usage_internal",
		"flags_commandlineflag",
		"int128",
		"synchronization",
		"scoped_set_env",
		"time",
		"status",
		"random_internal_randen_hwaes",
		"cord",
		"base",
		"flags_commandlineflag_internal",
		"random_distributions",
		"random_internal_randen",
		"strings",
		"strerror",
		"flags_config",
		"str_format_internal",
		"flags_program_name",
		"debugging_internal",
		"cordz_info",
		"bad_any_cast_impl",
		"cord_internal",
		"leak_check_disable",
		"raw_hash_set",
		"flags",
		"throw_delegate",
		"statusor",
		"stacktrace",
		"cordz_handle",
		"random_seed_gen_exception",
		"flags_internal",
		"flags_reflection",
		"exponential_biased",
		"city",
		"bad_optional_access",
		"flags_private_handle_accessor",
	}
	absl_linker_command = "-Wl,--start-group"
	for i, v in pairs(absl_libs) do
		absl_linker_command = absl_linker_command .. " -labsl_" .. v
	end
	absl_linker_command = absl_linker_command .. " -Wl,--end-group"
	
	add_ldflags(absl_linker_command)

	-- Add all libraries from vcpkg (just mined from a directory listing, excluding the absl libraries above)
	add_links(
		"grpc_upbdefs",
		"grpc++_unsecure",
		"upb",
		"address_sorting",
		"utf8_range",
		"grpcpp_channelz",
		"gpr",
		"prometheus-cpp-core",
		"grpc++",
		"prometheus-cpp-pull",
		"crypto",
		"ssl",
		"cares",
		"re2",
		"grpc++_error_details",
		"fmt",
		"z",
		"protobuf",
		"gtest",
		"civetweb-cpp",
		"upb_textformat",
		"protobuf-lite",
		"json",
		"upb_fastdecode",
		"upb_reflection",
		"grpc_plugin_support",
		"grpc++_alts",
		"grpc_unsecure",
		"spdlog",
		"civetweb",
		"grpc++_reflection",
		"protoc",
		"gmock",
		"grpc")

	add_files("src/Log.cpp", {cxxflags = compilerflags})
	add_files("evtc_rpc_server/**.cpp", {cxxflags = compilerflags})
	add_files("networking/**.cpp", {cxxflags = compilerflags})
	add_files("networking/**.proto")

	add_cxxflags("-fPIC")
	add_cxxflags("-ggdb3")
	add_cxxflags("-march=native")
	add_cxxflags("-Wextra", "-Weffc++", "-pedantic")
	add_cxxflags("-Werror=shadow", "-Werror=duplicate-decl-specifier", "-Werror=ignored-qualifiers", "-Werror=return-type")
	add_cxxflags("-Wno-format") -- unsigned long long vs unsigned long issues (linux is stupid...)
	add_cxxflags("-Wno-gnu-zero-variadic-macro-arguments", "-Wno-format-pedantic")
	add_ldflags("-fuse-ld=lld")
