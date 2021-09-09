# Building the project
### Prerequisites:
Visual Studio 2019
vcpkg (installed with visual studio integration)

### Clone the project
--recursive is important so that it clones submodules
```
git clone --recursive https://github.com/Krappa322/arcdps_healing_stats
```

### Build the addon
Open up arcdps_personal_stats.sln with Visual Studio. Choose mode (Release/Debug) and build the solution

### Running tests
Set test.vcxproj as startup project, and run "Local Windows Debugger". You can also run test.exe from in the output directory
