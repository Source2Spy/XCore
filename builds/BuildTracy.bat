##@echo OFF
cd %CD%

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



@echo ON

echo %INCLUDE%
cd "%CD%\..\dependencies\tracy\profiler\build\win32"
set INCLUDE="..\..\..\..\..\freetype\include;.\..\..\..\..\glfw-3.3.bin.WIN64\include;%INCLUDE%"
set LIB="..\..\..\..\freetype\win64;..\..\..\..\glfw-3.3.bin.WIN64\lib-vc2019;%LIB%"
REM use environment variables for INCLUDE and LIB values
##set UseEnv=true

echo zstad: Updating...
##"%VSPATH%\Common7\IDE\devenv.exe" "tracy.sln" /upgrade

"%MSBUILD%" "tracy.sln" /t:build /p:configuration=Release /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :PAUSE

####

:PAUSE
pause 