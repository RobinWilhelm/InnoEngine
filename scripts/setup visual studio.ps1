$SOURCE_PATH = "../"
if((Get-Variable 'VCPKG_ROOT' -ErrorAction SilentlyContinue) -eq $null){
    $env:VCPKG_ROOT = "./scripts/vcpkg/"
}
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
cmake -S $SOURCE_PATH --preset win64-default
Read-Host -Prompt "Press Enter to exit"