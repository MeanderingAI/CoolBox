param(
    [switch]$Run
)

$ErrorActionPreference = 'Stop'

$sourceDir = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $sourceDir 'build_windows'

. (Join-Path $PSScriptRoot 'movie_editor_ffmpeg_windows.ps1')
Initialize-MovieEditorFfmpeg -EnvironmentFile (Join-Path $buildDir 'movie_editor.env')

& cmake -S $sourceDir -B $buildDir -A x64 -DCOOLBOX_MOVIE_EDITOR_ONLY=ON
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& cmake --build $buildDir --config Debug --target movie_editor
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Run) {
    & (Join-Path $buildDir '_deliverables\apps\movie_editor\Debug\movie_editor.exe')
    exit $LASTEXITCODE
}
