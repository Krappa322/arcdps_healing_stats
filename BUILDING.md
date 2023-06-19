# Building the project
### Prerequisites:
Visual Studio 2022, Version 17.6.3 was working for me at the time of writing
vcpkg (installed with visual studio integration), Version 2023.04.15 was working for me at the time of writing
Python, Version 3.9 was working for me at the time of writing

### Clone the project
--recursive is important so that it clones submodules
```
git clone --recursive https://github.com/Krappa322/arcdps_healing_stats
```

### Build the addon
Open up arcdps_personal_stats.sln with Visual Studio. Choose mode (Release/Debug) and build the solution. You might have to rebuild only the "vcpkg_install_dependencies" project first before building the whole solution, to ensure that some of the tools needed for building (grpc, protobuf) are downloaded first.

### Running tests
Set test.vcxproj as startup project, and run "Local Windows Debugger". You can also run test.exe from in the output directory
