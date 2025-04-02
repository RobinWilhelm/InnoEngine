$SOURCE_PATH = "../"
Powershell.exe ./setEnv.ps1
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
cmake -S $SOURCE_PATH --preset win64-default
Read-Host -Prompt "Press Enter to exit"