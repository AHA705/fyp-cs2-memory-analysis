@echo off
setlocal

:: Directory containing this batch file
set "SCRIPT_DIR=%~dp0"

:: Paths relative to this script
set "DUMPER_SRC=%SCRIPT_DIR%..\tools\cs2-dumper"
set "DUMPER=%DUMPER_SRC%\target\release\cs2-dumper.exe"
set "OUTDIR=%SCRIPT_DIR%include\memory-offsets"
set "FILE_TYPES=hpp"

:: Check if cs2-dumper.exe exists, if not, build it
if not exist "%DUMPER%" (
    echo [INFO] cs2-dumper.exe not found, attempting to compile cs2-dumper from source.

    :: Check if cargo is available
    where cargo >nul 2>nul
    if %ERRORLEVEL% neq 0 (
        echo [FAILED] Cargo is not installed or not in PATH.
        endlocal
        exit /b 1
    )

    pushd "%DUMPER_SRC%"
    cargo build --release
    if %ERRORLEVEL% neq 0 (
        echo [FAILED] Failed to compile cs2-dumper.
        popd
        endlocal
        exit /b 1
    )
    popd
)

:: Run cs2-dumper
"%DUMPER%" --file-types "%FILE_TYPES%" --output "%OUTDIR%"
if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Offsets updated.
    if exist "%OUTDIR%\info.json" (
        echo --- info.json ---
        type "%OUTDIR%\info.json"
        echo --- end of info.json ---
    ) else (
        echo [WARNING] info.json not found in %OUTDIR%
    )
) else (
    echo [FAILED] cs2-dumper failed to update offsets.
)

endlocal