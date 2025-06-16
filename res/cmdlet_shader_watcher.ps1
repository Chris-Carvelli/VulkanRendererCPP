# shader_wathcer.ps1
#
# THIS SCRIPT IS NOT RECOMMENDED IN AN ACTUAL DEV ENVIRONMENT (see below)
#
# this script watches for file changes in the shader source
# directory and calls `glslang` whenever a file change is detected
# Script limitations:
# - does not check for shader include files
# - FileSystemWatcher is tricky, with multiple event fires and other
#   potentially problematic quirks. ATM, it seems to consistently raise
#   two `Changed` events on every file save, so skipping every other one
#   seems to be serviceable for now. Monitoring file creation is gonna
#   be MUCH more problematic
# 
# Other useful cmdlet
# - Get-EventSubscriber
# - Unregister-Event <ID>

$path = (pwd).Path + "\"

$shader_include_folders = $path + "shaders_include\"
$vulkan_version = 450
$preamble_text = '--preamble-text '
$path_src = $path + "shaders_src\"
$path_dst = $path + "shaders\"

$watcher = New-Object IO.FileSystemWatcher
$watcher.Path = $path_src
$watcher.Filter = "*.*"
$watcher.NotifyFilter = "LastWrite"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

$changeAction = {
    # skip every other notification
    if($event.EventIdentifier % 2 -eq 1)
    {
        $file_name = Split-Path $event.SourceEventArgs.Name -leaf
        Write-Host "---"
        Write-Host -ForegroundColor Green "["$event.EventIdentifier"]" $event.SourceEventArgs.ChangeType $file_name
        $file_path_src = $path_src + $file_name
        $file_path_dst = $path_dst + $file_name + ".spv"
glslang -I"$shader_include_folders" -V --glsl-version $vulkan_version -P"#extension GL_ARB_shading_language_include : require" $file_path_src -o $file_path_dst | Write-Host
    }
}

Register-ObjectEvent $Watcher -EventName "Changed" -Action $changeAction
