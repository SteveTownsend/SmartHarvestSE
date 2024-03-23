# Smart Harvest SE

https://www.nexusmods.com/skyrimspecialedition/mods/37091

## Building

The library builds using CMake. I don't use the Colorglass reportsitory for CommonLibSSE-NG because I found it hard to work out what was gpoing on. Instead, CMakeFiles.txt manually impoerts its dependencies using *FetchContent*.
Doing this brute force helped me understand CMake better, and makes it possible to experiment with other CommonLibSSE-NG variants. I don't plan to revert the build to use vcpkg. *CMakeFiles.txt* and *CMakePresets.json* retain vcpkg intergration for possible future change of heart.

CMake is confusing, no question. Things to know:
- it is a two-phase process: first *configure* to generate the build, then *build*
- once you have configured, you can only build that configuration until you change the configuration preset by removing the generated *CMakeFiles* and *CMakeCache.txt*, and re-running the configure step
- don't build in your source code directory, it clutters it up with cruft
- **multi-target generators** exist that allow build of different targets without reconfiguring, but that's just going to conuse me all over again.

1. Check out the code
2. Start a Visual Studio 20xx Powershell window to do the build
3. change directory to the SmartHarvestSE root directory you just checked out.
4. Configure using the appropriate preset *cmake -B ./build -S . --preset Logging-MSVC|Release-MSVC|Debug-MSVC*
5. Build using the matching **CMAKE_BUILD_TYPE**: *cmake --build ./build/ --config RelWithDebInfo|Release|Debug*

When you want to change the build you are working with, configure then build using the matching **CMAKE_BUILD_TYPE**.
Versioned build artifacts go into *./build/zip* in a subdirectory that matches the build config.
If you edit *CMakeFiles.txt* or other CMake files, manually delete *./build/CMakeCache.txt* and *./build/CMakefiles* to trigger full rebuild. This should not be necessary for vanila code edits.

## Dependencies
**CMake** minimum version per CMakeFiles.Txt
**Visual Studio 2022**, for its CMake Generator
**WinRAR** - rar.exe is used to compress the archive for release to Nexus. If you are just building locally, comment that part out at the end of *CmakeFiles.txt*

## Scripts
Scripts can be built using the Visual Studio Code Papyrus extension. There is a Papyrus project file **SkyrimSE.ppj** that should be edited according to your local setup.

_note: CMakeFiles is referring to all the generated CMakeFiles in your build directory... You are able to keep _deps around to save having to redownload the project _dependencies_
