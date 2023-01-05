import os
import subprocess
from typing import List, NamedTuple

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

VCPKG_ROOT = "/local/vcpkg/"
VCPKG_PATH = os.path.join(VCPKG_ROOT, "vcpkg")
TRIPLET = "x64-linux"
MANIFEST_ROOT = SCRIPT_DIR
INSTALL_ROOT = os.path.join(MANIFEST_ROOT, "vcpkg_installed", TRIPLET)
VERSIONS_FILE = os.path.join(SCRIPT_DIR, "evtc_rpc_server", "linux_versions_auto.h")

class DependencyInfo(NamedTuple):
	name: str
	version: str
	description: str

def get_dependency_versions() -> List[DependencyInfo]:
	vcpkg_output = subprocess.run(
		[VCPKG_PATH, "list", "--x-wait-for-lock", "--triplet", TRIPLET, "--vcpkg-root", VCPKG_ROOT, "--x-manifest-root", MANIFEST_ROOT, "--x-install-root", INSTALL_ROOT],
		capture_output=True, universal_newlines=True)
	assert vcpkg_output.returncode == 0

	res = []
	for line in vcpkg_output.stdout.splitlines():
		split = line.split()

		# Features are shown as separate entries but do not have versions, skip them
		if "[" in split[0]:
			continue

		res.append(DependencyInfo(
			name=split[0],
			version=split[1],
			description=split[2] if len(split) >= 3 else ""))
	return res

def generate_versions_file(deps: List[DependencyInfo]):
	longest_dep_name = max([len(dep.name) for dep in deps])
	with open(VERSIONS_FILE, "w") as file:
		file.write("#pragma once\n")
		file.write("#define DEPENDENCY_VERSIONS ")
		for dep in deps:
			file.write("\\\n")
			file.write(f"\"{dep.name: <{longest_dep_name}} {dep.version}\\n\"")



generate_versions_file(get_dependency_versions())