# Doing builds with an intermediate python file means the custom build step looks a bit cleaner, and we can also
# provide visual studio parseable errors so that they show up in the error list (instead of having to scroll through
# the build output)
import re, subprocess, sys

triplet = sys.argv[1]
protoc_path = r"..\vcpkg_installed\{}\tools\protobuf\protoc.exe".format(triplet)
grpc_plugin_path = r"..\vcpkg_installed\{}\tools\grpc\grpc_cpp_plugin.exe".format(triplet)
filename = sys.argv[2]
output_path = sys.argv[3]

args = [protoc_path, "--cpp_out={}".format(output_path), "--grpc_out={}".format(output_path), "--plugin=protoc-gen-grpc={}".format(grpc_plugin_path), filename]
#print(" ".join(args), file=sys.stderr)
result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

for line in result.stderr.splitlines():
	match = re.match(r"(.*?):(.*?):(.*?):(.*)", line)
	if match is not None:
		print("{}({},{}) : error PROTO : {}".format(match.group(1), match.group(2), match.group(3), match.group(4)), file=sys.stderr)

exit(result.returncode)
