# --- start.ps1 ---

function Import-BatchEnvironment {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [string]$Arguments = ""
    )

    if (-not (Test-Path $Path)) { throw "Batch file not found: $Path" }

    $temp = [System.IO.Path]::GetTempFileName()
    & cmd.exe /c "`"$Path`" $Arguments && set" > $temp

    Get-Content $temp | ForEach-Object {
        if ($_ -match '^(.*?)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }

    Remove-Item $temp -ErrorAction SilentlyContinue
}

# 1) Load VS dev env first (this modifies PATH)
if (-not $env:VSCMD_VER) {
    Import-BatchEnvironment "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" "-arch=x64"
}

# 2) Now prepend your desired folder using the *current* process PATH
$misc = "Y:\Development\Pantomir"
if (Test-Path $misc) {
    $procPath = [System.Environment]::GetEnvironmentVariable('Path','Process')
    if ($procPath -notmatch [regex]::Escape($misc)) {
        $newPath = "$misc;$procPath"
        [System.Environment]::SetEnvironmentVariable('Path', $newPath, 'Process')
        $env:Path = $newPath  # keep PS view in sync
    }
}

# 3) Project dir
Set-Location "Y:\Development\Pantomir"

