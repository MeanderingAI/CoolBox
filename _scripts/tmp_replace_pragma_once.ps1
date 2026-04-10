$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$extensions = @('.h', '.hpp', '.hh', '.hxx')
$headerFiles = Get-ChildItem -Path $root -Recurse -File | Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() }

foreach ($file in $headerFiles) {
    $content = [System.IO.File]::ReadAllText($file.FullName)
    if ($content -notmatch '(?m)^\uFEFF?\s*#pragma once\s*$') {
        continue
    }

    $newline = if ($content.Contains("`r`n")) { "`r`n" } else { "`n" }
    $stripped = [regex]::Replace($content, '(?m)^\uFEFF?\s*#pragma once\s*\r?\n?', '')
    $relative = $file.FullName.Substring($root.Length + 1) -replace '[^A-Za-z0-9]', '_'
    $guard = ('COOLBOX_' + $relative).ToUpper()
    $stripped = $stripped.TrimStart("`r", "`n")
    $updated = "#ifndef $guard${newline}#define $guard${newline}${newline}$stripped${newline}${newline}#endif  // $guard${newline}"
    [System.IO.File]::WriteAllText($file.FullName, $updated, [System.Text.UTF8Encoding]::new($false))
}
