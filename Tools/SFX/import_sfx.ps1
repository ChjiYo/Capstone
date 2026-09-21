# SoundSource/*.wav 를 /Game/UE_Asset/Sounds/Gen/ 에 USoundWave 로 임포트한다 (에디터를 닫은 상태에서 실행).
# 사용법:  powershell -ExecutionPolicy Bypass -File Tools\SFX\import_sfx.ps1
# -source 로 폴더를 주면 하위 폴더(SoundSource/)가 에셋 경로에 한 단계 더 붙어버려서, 파일 목록을 JSON 설정으로 직접 넘긴다.
$ErrorActionPreference = "Stop"

$engine  = "C:\Epic_Game\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$root    = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$project = Join-Path $root "Capstone.uproject"
$source  = Join-Path $root "SoundSource"
$log     = Join-Path $root "Saved\Logs\ImportSFX.log"
$json    = Join-Path $root "Saved\ImportSFX.json"

$files = Get-ChildItem $source -Filter *.wav | ForEach-Object { $_.FullName }
$settings = @{
    ImportGroups = @(@{
        GroupName       = "SFX"
        Filenames       = @($files)
        DestinationPath = "/Game/UE_Asset/Sounds/Gen"
        bReplaceExisting = $true
        bSkipReadOnly    = $true
    })
}
$settings | ConvertTo-Json -Depth 5 | Set-Content -Path $json -Encoding utf8

& $engine $project -run=ImportAssets -importsettings="$json" -nosourcecontrol -unattended -nopause -nosplash -stdout -abslog="$log"

Write-Host "완료. 로그: $log"
