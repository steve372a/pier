<# 
.SYNOPSIS
  Pier 多语言 ProFile 编写工具 (PowerShell)
.DESCRIPTION
  创建 profile.sque，为 PackageName、ProFile、Author 等字段提供多语言翻译。
  使用记事本编辑，语言种类不限。
.NOTES
  用法: powershell -ExecutionPolicy Bypass -File mkprofile.ps1 [-LangDir <path>]
  文件编码: UTF-8
#>

param([string]$LangDir = "")

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PierRoot = Split-Path -Parent $ScriptRoot
$Lang = @{}

function Load-LangStrings {
    param([string]$Path)
    $t = @{}; $k = $null
    Get-Content -Path $Path -Encoding UTF8 | ForEach-Object {
        $l = $_.Trim()
        if ($l -match '^\[(.+)\]$') { $k = $Matches[1] }
        elseif ($k -and $l -ne '') { $t[$k] = $l; $k = $null }
    }
    return $t
}
function G { param([string]$K); if ($Lang.ContainsKey($K)) { return $Lang[$K] }; return $K }

if (-not $LangDir) {
    $ini = Join-Path $PierRoot "etc\language.ini"
    if (Test-Path $ini) {
        $r = (Get-Content $ini -Encoding UTF8 -TotalCount 1).Trim()
        $LangDir = $r -replace '%PIER_ROOT%', $PierRoot
    }
}
if (-not $LangDir -or -not (Test-Path $LangDir)) {
    $LangDir = Join-Path $PierRoot "share\language\zh-CN"
}
$lf = Join-Path $LangDir "langtools.ini"
if (Test-Path $lf) { $Lang = Load-LangStrings $lf }

Write-Host "========================================"
Write-Host (G "mkprofile_banner")
Write-Host "========================================"
Write-Host ""
Write-Host (G "mkprofile_intro")
Write-Host ""

$contributors = Read-Host -Prompt (G "prompt_contributors")
$outName = Read-Host -Prompt ("$(G "prompt_output_name") [$(G "prompt_output_name_default")]")
if (-not $outName) { $outName = G "prompt_output_name_default" }

$template = @"
# $(G "mkprofile_template_header")
==================
[zh-CN]
PackageName: 
ProFile: 
Author:

[en-US]
PackageName: 
ProFile: 
Author:

[ja-JP]
PackageName: 
ProFile: 
Author:
"@

$tempFile = Join-Path $env:TEMP "pier_pro_$([System.IO.Path]::GetRandomFileName()).txt"
Set-Content -Path $tempFile -Value $template -Encoding UTF8
Write-Host ((G "notepad_waiting") -f "ProFile")
Start-Process notepad.exe -ArgumentList $tempFile -Wait

$edited = Get-Content -Path $tempFile -Encoding UTF8
Remove-Item -Path $tempFile -Force

$hdr = "# Language contributors: $contributors"
if (-not $contributors) { $hdr = "# Language contributors" }
$hdr = $hdr + "`r`n# Pip thanks you all!"
$outLines = @($hdr, "")

$curLang = $null
$kv = $null

foreach ($line in $edited) {
    $t = $line.Trim()
    if ($t -eq '' -or $t -match '^#') { continue }
    if ($t -match '^\[(.+)\]$') {
        if ($curLang -and $kv) {
            $outLines += "[$curLang]"
            foreach ($x in $kv) { $outLines += $x }
            $outLines += ""
        }
        $curLang = $Matches[1]
        $kv = @()
    } elseif ($curLang -and $t -match '^([^:]+):\s*(.*)$') {
        $kv += "$($Matches[1]): $($Matches[2])"
    }
}
if ($curLang -and $kv) {
    $outLines += "[$curLang]"
    foreach ($x in $kv) { $outLines += $x }
}

$outFile = "$outName.sque"
$outPath = Join-Path (Get-Location) $outFile
$content = $outLines -join "`r`n"
Set-Content -Path $outPath -Value $content -Encoding UTF8 -NoNewline

Write-Host ""
Write-Host ((G "success_generated") -f $outPath)
Write-Host ""
Write-Host (G "preview_label")
Write-Host "---"
Write-Host $content
Write-Host "---"