param(
    [string]$BridgeExe = ".\build\_deliverables\libraries\groups\app_assets\ai_chat\Debug\ai_chat_voice_bridge.exe",
    [string]$CadBuilderExe = ".\build\_deliverables\apps\cad_builder\Debug\cad_builder.exe",
    [string]$OutScript = "build\ai_chat_live_commands.txt",
    [string]$TranscriptFile = "",
    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RequiredPath {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Name
    )
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if ($null -eq $resolved) {
        throw "$Name not found: $Path"
    }
    return $resolved.Path
}

function Ensure-DirectoryForFile {
    param([Parameter(Mandatory=$true)][string]$FilePath)
    $parent = Split-Path -Path $FilePath -Parent
    if (-not [string]::IsNullOrWhiteSpace($parent) -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
}

function Capture-SpeechToTranscript {
    param([Parameter(Mandatory=$true)][string]$OutputTranscriptFile)

    Add-Type -AssemblyName System.Speech

    $engine = New-Object System.Speech.Recognition.SpeechRecognitionEngine
    $engine.LoadGrammar((New-Object System.Speech.Recognition.DictationGrammar))
    $engine.SetInputToDefaultAudioDevice()

    Write-Host "Live microphone capture started."
    Write-Host "Say CAD commands naturally (for example: 'add line', 'zoom in 100')."
    Write-Host "Say 'stop listening' to finish capture and launch cad_builder."

    $captured = New-Object System.Collections.Generic.List[string]

    while ($true) {
        $result = $engine.Recognize()
        if ($null -eq $result) {
            continue
        }

        $text = $result.Text.Trim()
        if ([string]::IsNullOrWhiteSpace($text)) {
            continue
        }

        Write-Host ("heard: " + $text)
        if ($text.ToLowerInvariant() -eq "stop listening") {
            break
        }

        $captured.Add($text)
    }

    $engine.Dispose()

    Ensure-DirectoryForFile -FilePath $OutputTranscriptFile
    Set-Content -LiteralPath $OutputTranscriptFile -Value $captured -Encoding UTF8

    Write-Host ("Saved transcript lines: " + $captured.Count)
}

$bridgePath = Resolve-RequiredPath -Path $BridgeExe -Name "ai_chat_voice_bridge"
$cadPath = Resolve-RequiredPath -Path $CadBuilderExe -Name "cad_builder"

$transcriptPath = $TranscriptFile
if ([string]::IsNullOrWhiteSpace($transcriptPath)) {
    $transcriptPath = "build\ai_chat_live_transcript.txt"
    Capture-SpeechToTranscript -OutputTranscriptFile $transcriptPath
} else {
    $transcriptPath = (Resolve-Path -LiteralPath $transcriptPath).Path
}

Ensure-DirectoryForFile -FilePath $OutScript

$bridgeArgs = @(
    "--transcript-file", $transcriptPath,
    "--out-script", $OutScript,
    "--run-cad-builder", $cadPath
)
if ($Quiet) {
    $bridgeArgs += "--quiet"
}

Write-Host "Running ai_chat_voice_bridge..."
& $bridgePath @bridgeArgs
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "ai_chat_voice_bridge failed with exit code $exitCode"
}

Write-Host "Voice-driven CAD launch complete."
