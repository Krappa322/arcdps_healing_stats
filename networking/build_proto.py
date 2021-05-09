# Doing builds with an intermediate python file means the custom build step looks a bit cleaner, and we can also
# provide visual studio parseable errors so that they show up in the error list (instead of having to scroll through
# the build output)
import re, subprocess, sys

protoc_path = r"Q:\Programming\vcpkg\installed\x64-windows\tools\protobuf\protoc"
grpc_plugin_path = r"Q:\Programming\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe"
filename = sys.argv[1]

args = [protoc_path, "--cpp_out=.", "--grpc_out=generate_mock_code=true:.", "--plugin=protoc-gen-grpc={}".format(grpc_plugin_path), filename]
result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

for line in result.stderr.splitlines():
	match = re.match(r"(.*?):(.*?):(.*?):(.*)", line)
	if match is not None:
		print("{}({},{}) : error PROTO : {}".format(match.group(1), match.group(2), match.group(3), match.group(4)))

exit(result.returncode)
