rule("protobuf")
	set_extensions(".proto")
	before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
		import("lib.detect.find_tool")

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
		local protoc = find_tool("protoc")

		batchcmds:show_progress(opt.progress, "${color.build.object}compiling.proto_source %s", sourcefile)
		batchcmds:vrunv(protoc.program, {"--cpp_out="..outputdir, "--grpc_out="..outputdir, "--plugin=protoc-gen-grpc="..grpc_cpp_plugin, "--proto_path="..basedir, targetfile})

		local lowest_mtime = nil
		for _, extension in pairs({".pb.cc", ".pb.h", ".grpc.pb.cc", ".grpc.pb.h"}) do
			file = path.join(outputdir, basename..extension)

			local mtime = os.mtime(file)
			if (lowest_mtime == nil) or (mtime < lowest_mtime) then
				lowest_mtime = mtime
			end
		end

		local depcache = target:dependfile(sourcefile.."_source")
		batchcmds:add_depfiles(sourcefile)
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


target("server")
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
		--table.insert(compilerflags, "-fno-sanitize=alignment")
		add_ldflags("-fsanitize=undefined")
		--add_ldflags("-fno-sanitize=alignment")

	elseif is_mode("release") then
		set_optimize("fastest")
		add_defines("NDEBUG")
		add_cxxflags("-flto")
		add_ldflags("-flto")
		add_ldflags("-fuse-linker-plugin")

	end

	add_links("grpc++", "protobuf", "gpr", "spdlog", "fmt", "absl_synchronization")

	add_files("src/Log.cpp", {cxxflags = compilerflags})
	add_files("evtc_rpc_server/**.cpp", {cxxflags = compilerflags})
	add_files("networking/**.cpp", {cxxflags = compilerflags})
	add_files("networking/**.cpp", {cxxflags = compilerflags})
	add_files("networking/**.proto")

	add_includedirs("arcdps_mock/arcdps-extension", "spdlog/include")

	if is_os("linux") then
		add_defines("LINUX")
		add_syslinks("pthread")
		add_cxxflags("-ggdb3")
	end

	add_cxxflags("-fPIC")
	add_cxxflags("-march=native")
	add_cxxflags("-Wextra", "-Weffc++", "-pedantic")
	add_cxxflags("-Werror=shadow", "-Werror=duplicate-decl-specifier", "-Werror=ignored-qualifiers", "-Werror=return-type")
	add_cxxflags("-Wno-format") -- unsigned long long vs unsigned long issues (linux is stupid...)
	add_cxxflags("-Wno-gnu-zero-variadic-macro-arguments", "-Wno-format-pedantic")
	add_ldflags("-fuse-ld=lld")
