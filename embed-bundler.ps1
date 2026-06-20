param(
    [string]$gameScript,
    [string]$outputCpp
)

# Determine script directory from our own location
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Debug log
$debugLog = "$env:TEMP\uclang_bundler_debug.log"
"scriptDir=[$scriptDir]" | Out-File -FilePath $debugLog
"gameScript=[$gameScript]" | Out-File -FilePath $debugLog -Append
"outputCpp=[$outputCpp]" | Out-File -FilePath $debugLog -Append

if ([string]::IsNullOrEmpty($gameScript)) { throw "gameScript is empty" }
if ([string]::IsNullOrEmpty($outputCpp)) { throw "outputCpp is empty" }

# Ordered list of stdlib files to bundle
$stdlibFiles = @(
    'stdlib/mathf.uc',
    'stdlib/transform.uc',
    'stdlib/gameobject.uc',
    'stdlib/time.uc',
    'stdlib/input.uc',
    'stdlib/camera_2d.uc',
    'stdlib/ui_canvas.uc',
    'stdlib/graphics.uc',
    'stdlib/debug.uc',
    'stdlib/ecs.uc',
    'stdlib/particles.uc',
    'stdlib/pathfind.uc',
    'stdlib/events.uc',
    'stdlib/coroutine.uc',
    'stdlib/component.uc',
    'stdlib/animator.uc',
    'stdlib/prefab.uc',
    'stdlib/asset.uc'
)

$varDecls = @()
$concatLines = @()

# Process stdlib files
foreach ($relPath in $stdlibFiles) {
    $fullPath = Join-Path -Path $scriptDir -ChildPath $relPath
    $varName = ('g_uc_' + $relPath) -replace '[^a-zA-Z0-9]', '_'
    $varName = $varName -replace '_uc$', ''

    if (Test-Path -LiteralPath $fullPath) {
        $content = [System.IO.File]::ReadAllText($fullPath)
        $comment = "# source: $relPath"
        $varDecls += "const char* $varName = R""uc($comment`n$content)uc"";"
        $concatLines += "    fullScript += $varName;"
    } else {
        Write-Warning "Missing stdlib file: $fullPath"
    }
}

# Process game script
$gameContent = [System.IO.File]::ReadAllText($gameScript)
$gameVarName = 'g_uc_game'
$gameComment = "# game script"
$varDecls += "const char* ${gameVarName} = R""uc($gameComment`n$gameContent)uc"";"
$concatLines += "    fullScript += $gameVarName;"

# Read template and replace markers
$templatePath = Join-Path -Path $scriptDir -ChildPath 'uclang_app_template.cpp'
$tmpl = [System.IO.File]::ReadAllText($templatePath)
$out = $tmpl.Replace('// FILE_CONTENTS', $varDecls -join "`r`n")
$out = $out.Replace('// FILE_CONCAT', $concatLines -join "`r`n")

# Write output
[System.IO.File]::WriteAllText($outputCpp, $out, [System.Text.Encoding]::UTF8)
Write-Host "Generated $outputCpp with $($varDecls.Count) embedded files"
