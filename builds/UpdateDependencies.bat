@echo OFF
setlocal enabledelayedexpansion
cd %0/..

rem ------------------------------------------------------------------------------------------------------
rem Find Visual Studio MSBuild
rem ------------------------------------------------------------------------------------------------------
echo FINDING VISUAL STUDIO 
for /f "usebackq tokens=*" %%i in (`.\..\bin\vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    SET MSBUILD=%%i
)

for /f "usebackq tokens=1* delims=: " %%i in (`.\..\bin\vswhere -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
    if /i "%%i"=="installationPath" set VSPATH=%%j
)

IF EXIST "%MSBUILD%" GOTO :DOWNLOAD_DEPENDENCIES
echo Failed to find MSBuild!!! 
GOTO :PAUSE

rem ------------------------------------------------------------------------------------------------------
rem Download all dependencies
rem ------------------------------------------------------------------------------------------------------
:DOWNLOAD_DEPENDENCIES
echo DOWNLOADING DEPENDENCIES

rmdir "../dependencies/freetype" /S /Q
git clone https://github.com/ubawurinna/freetype-windows-binaries.git "../dependencies/freetype"

rmdir "../dependencies/glfw-3.3.bin.WIN64" /S /Q
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('glfw-3.3.bin.WIN64.zip', '../dependencies'); }"

rmdir "../dependencies/properties" /S /Q
git clone https://gitlab.com/LIONant/properties.git "../dependencies/properties"

rmdir "../dependencies/span" /S /Q
git clone https://github.com/tcbrindle/span.git "../dependencies/span"

rmdir "../dependencies/meow_hash_cpp" /S /Q
git clone https://github.com/RedSpah/meow_hash_cpp "../dependencies/meow_hash_cpp"

rmdir "../dependencies/tracy" /S /Q
git clone https://bitbucket.org/wolfpld/tracy/src/master/ "../dependencies/tracy"
cd "../dependencies/tracy"
git checkout 5e8b2a0
cd %0/..

rmdir "../dependencies/freetype" /S /Q
git clone https://github.com/ubawurinna/freetype-windows-binaries.git "../dependencies/freetype"

rem This may be needed at some point but right now we are not looking for too high speed of Compression.
rem A better ratio with still fast decompression is prefer.
rem rmdir "../dependencies/lz4" /S /Q
rem git clone https://github.com/lz4/lz4.git "../dependencies/lz4"

rmdir "../dependencies/zstd" /S /Q
git clone https://github.com/facebook/zstd.git "../dependencies/zstd"

rem ------------------------------------------------------------------------------------------------------
rem Compilations
rem ------------------------------------------------------------------------------------------------------
:COMPILATION
echo COMPILING DEPENDENCIES

echo zstad: Updating...
"%VSPATH%\Common7\IDE\devenv.exe" "%CD%\..\dependencies\zstd\build\VS2010\zstd.sln" /upgrade

echo zstad: Compiling...
"%MSBUILD%" "%CD%\..\dependencies\zstd\build\VS2010\zstd.sln" /t:libzstd /p:configuration=Release /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :PAUSE
"%MSBUILD%" "%CD%\..\dependencies\zstd\build\VS2010\zstd.sln" /t:libzstd /p:configuration=Debug /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :PAUSE

rem ------------------------------------------------------------------------------------------------------
rem Done
rem ------------------------------------------------------------------------------------------------------
:DONE
echo DONE!!

:PAUSE
pause