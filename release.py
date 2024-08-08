import asyncio
import datetime
import os
import re
import shutil
import string
import subprocess
from typing import List

MSBUILD_PATH = r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe"
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
RELEASES_PATH = os.path.join(SCRIPT_PATH, "..", "arcdps_personal_stats_releases")
BUILD_PATH = os.path.join(SCRIPT_PATH, "x64") # Gets cleaned automatically, be careful what path you put in
BUILD_ALL_PROJECT = "arcdps_personal_stats_build_all.proj"
ARCHVING_TARGETS = ["arcdps_healing_stats.dll", "arcdps_healing_stats.pdb"]
TEST_BINARY_NAME = "test.exe"


CONFIGURATIONS = ["Debug", "Release"]
START = datetime.datetime.now()

def Progress(pStatus: str):
	time_diff = datetime.datetime.now() - START
	print("{:03}.{:06} {}".format(int(time_diff.total_seconds()), time_diff.microseconds, pStatus))

def ExtractInternalVersion(pVersionString: str) -> List[int]:
	result: List[int] = []
	for token_raw in pVersionString.split("."):
		token = "".join([char for char in token_raw if char in string.digits])
		result.append(int(token))
	assert len(result) == 3, "Version must include 3 tokens"
	Progress("Parsed version")
	return result

def ChangeResourceVersion(pVersion: List[int]) -> None:
	with open(os.path.join(SCRIPT_PATH, "arcdps_personal_stats.rc"), "r+") as file:
		fulldata = file.read()

		fulldata, replace_count = re.subn(
			r" FILEVERSION \d+,\d+,\d+,\d+",
			r" FILEVERSION {},{},{},1".format(pVersion[0], pVersion[1], pVersion[2]),
			fulldata)
		assert replace_count == 1

		fulldata, replace_count = re.subn(
			r" PRODUCTVERSION \d+,\d+,\d+,\d+",
			r" PRODUCTVERSION {},{},{},1".format(pVersion[0], pVersion[1], pVersion[2]),
			fulldata)
		assert replace_count == 1

		fulldata, replace_count = re.subn(
			r'            VALUE "FileVersion", "\d+.\d+.\d+.\d+"',
			r'            VALUE "FileVersion", "{}.{}.{}.1"'.format(pVersion[0], pVersion[1], pVersion[2]),
			fulldata)
		assert replace_count == 1

		fulldata, replace_count = re.subn(
			r'            VALUE "ProductVersion", "\d+.\d+.\d+.\d+"',
			r'            VALUE "ProductVersion", "{}.{}.{}.1"'.format(pVersion[0], pVersion[1], pVersion[2]),
			fulldata)
		assert replace_count == 1

		file.seek(0)
		file.write(fulldata)
		file.truncate()
	Progress("Changed version in resource file")

# Returns an absolute path to the newly created directory
def CreateReleaseDirectory(pVersionString: str) -> str:
	path = os.path.join(RELEASES_PATH, pVersionString)
	os.mkdir(path)
	return path

def Build(pReleaseDirectory: str, pRebuild: bool):
	if pRebuild == True:
		Progress("Cleaning build")
		try:
			shutil.rmtree(BUILD_PATH)
		except FileNotFoundError:
			pass

	Progress("Starting build")
	build_output = subprocess.run(
		[MSBUILD_PATH, os.path.join(SCRIPT_PATH, BUILD_ALL_PROJECT), "-maxCpuCount", "-verbosity:diagnostic", "-consoleLoggerParameters:PerformanceSummary"],
		capture_output=True)
	Progress("Finished build")

	os.makedirs(os.path.join(pReleaseDirectory, "BuildLogs"), exist_ok=True)
	with open(os.path.join(pReleaseDirectory, "BuildLogs", "release_build_stdout.log"), "wb") as file:
		file.write(build_output.stdout)
	with open(os.path.join(pReleaseDirectory, "BuildLogs", "release_build_stderr.log"), "wb") as file:
		file.write(build_output.stderr)

	if (len(build_output.stderr) > 0):
		print(build_output.stderr.decode("utf8"))
	assert build_output.returncode == 0, "Build failed"

	Progress("Finished saving build logs")

def SetupTestDirectory(pConfiguration: str):
	shutil.copytree(
		os.path.join(SCRIPT_PATH, "test", "xevtc_logs"),
		os.path.join(BUILD_PATH, pConfiguration, "xevtc_logs"),
		dirs_exist_ok=True
	)
	Progress("Setup test directory for {}".format(pConfiguration))

async def _RunTests(pReleaseDirectory: str, pConfiguration: str):
	process = await asyncio.create_subprocess_exec(
		os.path.join(BUILD_PATH, pConfiguration, TEST_BINARY_NAME),
		cwd=os.path.join(BUILD_PATH, pConfiguration),
		stdout=asyncio.subprocess.PIPE,
		stderr=asyncio.subprocess.PIPE)
	Progress("Started tests for {}".format(pConfiguration))
	stdout, stderr = await process.communicate()
	Progress("Finished tests for {}".format(pConfiguration))

	with open(os.path.join(pReleaseDirectory, "TestLogs", "{}_tests_stdout.log".format(pConfiguration)), "wb") as file:
		file.write(stdout)
	with open(os.path.join(pReleaseDirectory, "TestLogs", "{}_tests_stderr.log".format(pConfiguration)), "wb") as file:
		file.write(stderr)

	Progress("Finished saving test logs for {}".format(pConfiguration))
	assert process.returncode == 0, "Tests failed for {}".format(pConfiguration)

async def Test(pReleaseDirectory: str):
	os.makedirs(os.path.join(pReleaseDirectory, "TestLogs"), exist_ok=True)

	for configuration in CONFIGURATIONS:
		SetupTestDirectory(configuration)
		await _RunTests(pReleaseDirectory, configuration)

	# Can't run in parallel since the tests bind to ports and thus interfere with eachother
	#await asyncio.gather(*[_RunTests(pReleaseDirectory, configuration) for configuration in CONFIGURATIONS])

def Archive(pReleaseDirectory: str):
	Progress("Archiving binaries")
	for configuration in CONFIGURATIONS:
		os.mkdir(os.path.join(pReleaseDirectory, configuration))
		for target in ARCHVING_TARGETS:
			shutil.copy2(
				os.path.join(BUILD_PATH, configuration, target),
				os.path.join(pReleaseDirectory, configuration, target))

	Progress("Archiving complete")

def Do_Release(pVersionString: str):
	version_internal = ExtractInternalVersion(pVersionString)
	release_directory = CreateReleaseDirectory(pVersionString)
	ChangeResourceVersion(version_internal)
	Build(release_directory, True)
	asyncio.run(Test(release_directory))
	Archive(release_directory)
	Progress("Release done")

def Do_Test():
	Build(BUILD_PATH, False)
	asyncio.run(Test(BUILD_PATH))
	Progress("Do_Test done")

#Do_Test()
Do_Release("v2.10.rc1")