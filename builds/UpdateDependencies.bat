@echo OFF
setlocal enabledelayedexpansion
cd %cd%
set XCORE_PATH="%cd%"

rem --------------------------------------------------------------------------------------------------------
rem To write line Colors
rem More info: https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/write-host?view=powershell-6
rem --------------------------------------------------------------------------------------------------------
rem > Black
rem > DarkBlue
rem > DarkGreen
rem > DarkCyan
rem > DarkRed
rem > DarkMagenta
rem > DarkYellow
rem > Gray
rem > DarkGray
rem > Blue
rem > Green
rem > Cyan
rem > Red
rem > Magenta
rem > Yellow
rem > White
rem use -fore instead of -foregroundcolor
rem use -back instead of -backgroundcolor
rem So if you want yellow text on a blue background you type
rem      powershell write-host -fore Cyan This is Cyan text
rem      powershell write-host -back Red  This is Red background
rem --------------------------------------------------------------------------------------------------------

rem --------------------------------------------------------------------------------------------------------
rem Set the color of the terminal to blue with yellow text
rem --------------------------------------------------------------------------------------------------------
COLOR 8E
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore Cyan Welcome I am your XCORE dependency updater bot, let me get to work...
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
echo.

powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE - FINDING VISUAL STUDIO / MSBuild
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
cd /d %XCORE_PATH%

for /f "usebackq tokens=*" %%i in (`.\..\bin\vswhere -version 16.0 -sort -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    SET MSBUILD=%%i
)

for /f "usebackq tokens=1* delims=: " %%i in (`.\..\bin\vswhere -version 16.0 -sort -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
    if /i "%%i"=="installationPath" set VSPATH=%%j
)

IF EXIST "%MSBUILD%" ( 
    echo VISUAL STUDIO VERSION: "%MSBUILD%"
    echo INSTALLATION PATH: "%VSPATH%"
    GOTO :DOWNLOAD_DEPENDENCIES
    )
powershell write-host -fore Red Failed to find VS2019 MSBuild!!! 
GOTO :ERROR


:DOWNLOAD_DEPENDENCIES
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE - DOWNLOADING DEPENDENCIES
powershell write-host -fore White ------------------------------------------------------------------------------------------------------

echo.
rmdir "../dependencies/tracy" /S /Q
git clone --recursive https://github.com/wolfpld/tracy "../dependencies/tracy"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem Forcely change the a define to allow tracy editor to work
powershell -Command "(gc "%CD%\..\dependencies\tracy\profiler\build\win32\Tracy.vcxproj") -replace 'IMGUI_ENABLE_FREETYPE', 'IMGUI_ENABLE_FREETYPE__' | Out-File -encoding ASCII "%CD%\..\dependencies\tracy\profiler\build\win32\Tracy.vcxproj""
if %ERRORLEVEL% GEQ 1 goto :ERROR


echo.
rmdir "../dependencies/properties" /S /Q
git clone https://gitlab.com/LIONant/properties.git "../dependencies/properties"
if %ERRORLEVEL% GEQ 1 goto :ERROR

echo.
rmdir "../dependencies/span" /S /Q
git clone https://github.com/tcbrindle/span.git "../dependencies/span"
if %ERRORLEVEL% GEQ 1 goto :ERROR

echo.
rmdir "../dependencies/meow_hash_cpp" /S /Q
git clone https://github.com/RedSpah/meow_hash_cpp "../dependencies/meow_hash_cpp"
if %ERRORLEVEL% GEQ 1 goto :ERROR

echo.
rmdir "../dependencies/zstd" /S /Q
git clone https://github.com/facebook/zstd.git "../dependencies/zstd"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem This may be needed at some point but right now we are not looking for too high speed of Compression.
rem A better ratio with still fast decompression is prefer.
rem rmdir "../dependencies/lz4" /S /Q
rem git clone https://github.com/lz4/lz4.git "../dependencies/lz4"

:COMPILATION
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE - COMPILING DEPENDENCIES
powershell write-host -fore White ------------------------------------------------------------------------------------------------------

powershell write-host -fore Cyan zstad: Updating...
rem start "Updating" "%VSPATH%\Common7\IDE\devenv.exe" "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj" /upgrade & timeout \t 30
"%VSPATH%\Common7\IDE\devenv.exe" "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj" /upgrade
if %ERRORLEVEL% GEQ 1 goto :ERROR


rem Forcely change the warning as errors as false
powershell -Command "(gc "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj") -replace '<TreatWarningAsError>true</TreatWarningAsError>', '<TreatWarningAsError>false</TreatWarningAsError>' | Out-File -encoding ASCII "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj""

echo.
powershell write-host -fore Cyan zstad Release: Compiling...
"%MSBUILD%" "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj" /p:configuration=Release /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :ERROR

echo.
powershell write-host -fore Cyan zstad Debug: Compiling...
"%MSBUILD%" "%CD%\..\dependencies\zstd\build\VS2010\libzstd\libzstd.vcxproj" /p:configuration=Debug /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :ERROR

:DONE
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE - SUCCESSFULLY DONE!!
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
goto :PAUSE

:ERROR
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------
powershell write-host -fore Red XCORE - FAILED!!
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------

:PAUSE
rem if no one give us any parameters then we will pause it at the end, else we are assuming that another batch file called us
if %1.==. pause