# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
[CmdletBinding()]
param (
    [Parameter()]
    [string]
    $IwyuBinDir,

    [Parameter()]
    [string]
    $Python
)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot/..

if (!$IwyuBinDir) {
    $cmd = Get-Command "iwyu" -ErrorAction SilentlyContinue
    if ($cmd) {
        $IwyuBinDir = $cmd.Source
    }
}
if (!$IwyuBinDir) {
    Write-Error "Please specify IwyuBinDir - could not find on path"
    return -1
}

if (!$Python) {
    $cmd = Get-Command "python3" -ErrorAction SilentlyContinue
    if ($cmd) {
        $Python = $cmd.Source
    }
}
if (!$Python) {
    $cmd = Get-Command "python" -ErrorAction SilentlyContinue
    if ($cmd) {
        $Python = $cmd.Source
    }
}
if (!$Python) {
    Write-Error "Please specify Python - could not find on path"
    return -1
}

$iwyuShareDir = Get-Item -Path $IwyuBinDir/../share/include-what-you-use

function Create-Args {

    [CmdletBinding()]
    Param(

        [Parameter(ValueFromPipeline)]
        $FullName
    )

    process {
        $item = Get-Item $FullName
        Write-Output '-Xiwyu'
        Write-Output ("--mapping_file=" + $item.FullName)
    }
}

$python_args = @(
    (Join-Path $IwyuBinDir "iwyu_tool.py") 
    "-p"
    "build"
    "src/xrt"
    "--"
)
$python_args = $python_args + (Join-Path $iwyuShareDir 'iwyu.gcc.imp' | Create-Args)
$python_args = $python_args + (Get-ChildItem -Path $iwyuShareDir -Filter "*intrinsics.imp" | Create-Args)
$python_args = $python_args + (Get-ChildItem -Path $iwyuShareDir -Filter "libcxx.imp" | Create-Args)
Write-Host $python_args
$our_mapping_file = Join-Path (Get-Location) scripts/mapping.imp

$python_args = $python_args + @("-Xiwyu", "--mapping_file=$our_mapping_file")
# & $Python (Join-Path $IwyuBinDir "iwyu_tool.py") -p build src/xrt -- -Xiwyu "--mapping_file=$our_mapping_file" @python_args | Tee-Object -FilePath iwyu.txt -Encoding utf8

Start-Process -FilePath $Python -ArgumentList $python_args -PassThru -Wait -NoNewWindow

Pop-Location

