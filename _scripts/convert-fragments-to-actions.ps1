# Script to convert workflow fragments to composite actions
# This fixes the "Can't find 'action.yml'" error

$fragmentsDir = ".github/workflows/fragments"
$fragmentFiles = Get-ChildItem -Path $fragmentsDir -Filter "*.yaml" -File

foreach ($file in $fragmentFiles) {
    Write-Host "Processing $($file.Name)..."

    # Read the file content
    $content = Get-Content -Path $file.FullName -Raw

    # Skip if already converted (contains 'using: composite')
    if ($content -match "using:\s*composite") {
        Write-Host "  Already a composite action, skipping."
        continue
    }

    # Get base name without extension
    $baseName = $file.BaseName

    # Create subdirectory for this action
    $actionDir = Join-Path $fragmentsDir $baseName
    New-Item -ItemType Directory -Path $actionDir -Force | Out-Null

    # Check if it starts with 'steps:' (invalid for composite action)
    if ($content -match "^steps:\s*$") {
        # Add composite action metadata
        $actionName = $baseName -replace '-', ' '
        $newContent = "# Composite action: $baseName`n"
        $newContent += "# Auto-converted from workflow fragment`n`n"
        $newContent += "name: '$actionName'`n"
        $newContent += "description: 'Composite action for $baseName'`n"
        $newContent += "runs:`n"
        $newContent += "  using: 'composite'`n"
        $newContent += "  steps:`n"

        # Remove the original 'steps:' line
        $content = $content -replace "^steps:\s*`n", ""
        $content = $newContent + $content

        # Add shell: bash to all run steps
        $content = $content -replace "(\s+)run:\s*\|", "`$1shell: bash`n`$1run: |"
        $content = $content -replace "(\s+)run:\s*>", "`$1shell: bash`n`$1run: >"
        $content = $content -replace "(\s+)run:\s*([^|>])", "`$1shell: bash`n`$1run: `$2"
    }

    # Write to action.yaml in the subdirectory
    $actionFile = Join-Path $actionDir "action.yaml"
    Set-Content -Path $actionFile -Value $content -NoNewline

    # Remove the original file
    Remove-Item -Path $file.FullName -Force

    Write-Host "  Converted to $actionDir/action.yaml"
}

Write-Host "`nConversion complete!"
Write-Host "Now updating workflow references..."

# Update references in workflow files
$workflowFiles = Get-ChildItem -Path ".github/workflows" -Filter "*.yaml" -File

foreach ($workflow in $workflowFiles) {
    $content = Get-Content -Path $workflow.FullName -Raw
    $originalContent = $content

    # Replace .yaml extensions with directory paths
    $content = $content -replace "uses:\s*\.\/\.github\/workflows\/fragments\/([^/\s]+)\.yaml", "uses: ./.github/workflows/fragments/`$1"

    if ($content -ne $originalContent) {
        Set-Content -Path $workflow.FullName -Value $content -NoNewline
        Write-Host "Updated references in $($workflow.Name)"
    }
}

Write-Host "`nAll done!"
