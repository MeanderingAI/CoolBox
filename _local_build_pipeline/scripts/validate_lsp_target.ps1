param(
    [Parameter(Mandatory = $true)]
    [string]$JobName,

    [Parameter(Mandatory = $true)]
    [string]$BinaryTarget,

    [Parameter(Mandatory = $true)]
    [string]$TestTarget,

    [Parameter(Mandatory = $true)]
    [string]$BinaryName
)

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pipelineRoot = Split-Path -Parent $scriptDir
$repoRoot = Split-Path -Parent $pipelineRoot
$repoMount = $repoRoot -replace '\\', '/'
$outDir = Join-Path $repoRoot "_local_build_pipeline/out/$JobName"
$outMount = $outDir -replace '\\', '/'
$image = if ($env:COOLBOX_LOCAL_IMAGE) { $env:COOLBOX_LOCAL_IMAGE } else { 'coolbox-local-linux-ci:latest' }
$hostLog = Join-Path $outDir 'host-command.log'
$hostExit = Join-Path $outDir 'host-exit.txt'

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Set-Content -Path $hostLog -Value "Starting job $JobName"

function Write-HostLog {
    param([string]$Message)

    Add-Content -Path $hostLog -Value $Message
}

Write-HostLog "Inspecting image $image"
docker image inspect $image *>> $hostLog
if ($LASTEXITCODE -ne 0) {
    Write-HostLog "Building image $image"
    docker build -t $image -f "$repoMount/_local_build_pipeline/docker/linux-ci.Dockerfile" $repoMount *>> $hostLog
    if ($LASTEXITCODE -ne 0) {
        Set-Content -Path $hostExit -Value "EXIT=$LASTEXITCODE"
        throw "Failed to build local CI image '$image'."
    }
}

Write-HostLog "Running docker validation container"
docker run --rm -t `
    -e CI=true `
    -e GITHUB_ACTIONS=false `
    -e GITHUB_WORKSPACE=/workspace `
    -e HOST_OUT=/hostout `
    -v "${repoMount}:/repo" `
    -v "${outMount}:/hostout" `
    -w /workspace `
    $image `
    bash /repo/_local_build_pipeline/scripts/container_entrypoint.sh $JobName `
    bash /workspace/_local_build_pipeline/scripts/jobs/build-lsp-target.sh $BinaryTarget $TestTarget $BinaryName *>> $hostLog

Write-HostLog "Docker exit code: $LASTEXITCODE"
Set-Content -Path $hostExit -Value "EXIT=$LASTEXITCODE"

if ($LASTEXITCODE -ne 0) {
    throw "LSP validation job '$JobName' failed."
}