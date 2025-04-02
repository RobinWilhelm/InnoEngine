$SOURCE_PATH = "../"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
cmake -S $SOURCE_PATH --preset win64-default
Read-Host -Prompt "Press Enter to exit"