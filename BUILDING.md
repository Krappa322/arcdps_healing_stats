# Building the project
### Prerequisites:
Visual Studio 2019
vcpkg (installed with visual studio integration)

### Clone the project
--recursive is important so that it clones submodules
```
git clone --recursive https://github.com/Krappa322/arcdps_healing_stats
```

### Build spdlog
cmake path might look different depending on system, change it as necessary
```
mkdir arcdps_healing_stats\spdlog\build_windows
cd arcdps_healing_stats\spdlog\build_windows

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ..

# Change project files to be static linked instead (needed to link the project properly)
powershell -Command "(gc spdlog.vcxproj) -replace 'MultiThreadedDebugDLL', 'MultiThreadedDebug' -replace 'MultiThreadedDLL', 'MultiThreaded' | Out-File -encoding ASCII spdlog.vcxproj"
powershell -Command "(gc example\example.vcxproj) -replace 'MultiThreadedDebugDLL', 'MultiThreadedDebug' -replace 'MultiThreadedDLL', 'MultiThreaded' | Out-File -encoding ASCII example\example.vcxproj"

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config Debug
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config Release

```

### Build the addon
Open up arcdps_personal_stats.sln with Visual Studio. Choose mode (Release/Debug) and build the solution

### Running tests
Set test.vcxproj as startup project, and run "Local Windows Debugger". You can also run test.exe from in the output directory
