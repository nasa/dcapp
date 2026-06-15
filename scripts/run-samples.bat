@echo off
setlocal

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%" >nul
set "DCAPP_HOME=%CD%"
popd >nul

if "%~1"=="" (
    set "TIMEOUT_SECONDS=5"
) else (
    set "TIMEOUT_SECONDS=%~1"
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "$homeDir = '%DCAPP_HOME%';" ^
  "$timeoutSeconds = [double]'%TIMEOUT_SECONDS%';" ^
  "if ($timeoutSeconds -le 0) { Write-Host 'Usage: scripts\run-samples.bat [timeout_seconds]'; exit 2 }" ^
  "$samplesDir = Join-Path $homeDir 'samples';" ^
  "$runDir = Join-Path $homeDir 'pilotlight\out';" ^
  "$exe = Join-Path $runDir 'pilot_light.exe';" ^
  "if (!(Test-Path $exe)) { Write-Host \"Missing $exe. Run scripts\build.bat first.\"; exit 1 }" ^
  "$samples = Get-ChildItem -Path $samplesDir -Directory | ForEach-Object { Get-ChildItem -Path $_.FullName -Filter '*.xml' -File } | Sort-Object FullName;" ^
  "if ($samples.Count -eq 0) { Write-Host 'No samples found.'; exit 1 }" ^
  "$pass = 0; $fail = 0; $skip = 0;" ^
  "Write-Host \"Running $($samples.Count) sample XML files with ${timeoutSeconds}s timeout each.\"; Write-Host '';" ^
  "foreach ($sample in $samples) {" ^
  "  $sampleName = Split-Path $sample.DirectoryName -Leaf;" ^
  "  $rel = $sample.FullName.Substring($homeDir.Length).TrimStart('\', '/');" ^
  "  if ($sampleName -eq 'bad-sample') { Write-Host \"[SKIP] $rel (intentional invalid sample)\"; $skip++; continue }" ^
  "  Write-Host \"[RUN ] $rel\";" ^
  "  $proc = Start-Process -FilePath $exe -WorkingDirectory $runDir -ArgumentList @('-a', 'dcapp', $sample.FullName) -PassThru;" ^
  "  $completed = $proc.WaitForExit([int][Math]::Ceiling($timeoutSeconds * 1000));" ^
  "  if (!$completed) {" ^
  "    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue;" ^
  "    Write-Host \"[ OK ] $rel reached timeout\"; $pass++;" ^
  "  } elseif ($proc.ExitCode -eq 0) {" ^
  "    Write-Host \"[ OK ] $rel exited normally\"; $pass++;" ^
  "  } else {" ^
  "    Write-Host \"[FAIL] $rel exited with status $($proc.ExitCode)\"; $fail++;" ^
  "  }" ^
  "  Write-Host '';" ^
  "}" ^
  "Write-Host \"Summary: $pass passed, $fail failed, $skip skipped.\";" ^
  "if ($fail -ne 0) { exit 1 }"

exit /b %ERRORLEVEL%
