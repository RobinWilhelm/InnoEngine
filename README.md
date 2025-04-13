# InnoEngine
Personal education project  
Not intended for any productional use

##Prerequisites
- Cmake (3.30 or higher)
- VCPKG (directory specified by environment variable VCPKG_ROOT)
##Build
- run "\scripts\setup visual studio.ps1" to generate visual studio project (cmake -S $SOURCE_PATH --preset win64-default)
- "cd build/win64-default"
- "cmake --build ."
- "ctest"
