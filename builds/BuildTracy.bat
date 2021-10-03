@echo OFF
setlocal enabledelayedexpansion
cd %cd%
set XCORE_PATH=%cd%

rem --------------------------------------------------------------------------------------------------------
rem Set the color of the terminal to blue with yellow text
rem --------------------------------------------------------------------------------------------------------
COLOR 8E
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore Cyan Welcome I am your XCORE (BUILD TRACY) dependency updater bot, let me get to work...
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
echo.

powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE (BUILD TRACY) - FINDING VISUAL STUDIO / MSBuild
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
cd /d %XCORE_PATH%
for /f "usebackq tokens=*" %%i in (`.\..\bin\vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    SET MSBUILD=%%i
    GOTO :BREAK_OUT
)
:BREAK_OUT

for /f "usebackq tokens=1* delims=: " %%i in (`.\..\bin\vswhere -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
    if /i "%%i"=="installationPath" set VSPATH=%%j
)

IF EXIST "%MSBUILD%" ( 
    echo OK 
    GOTO :DOWNLOAD_DEPENDENCIES
    )
echo Failed to find MSBuild!!! 
GOTO :ERROR

:COMPILATION
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE (BUILD TRACY) - COMPILING DEPENDENCIES
powershell write-host -fore White ------------------------------------------------------------------------------------------------------

cd "..\dependencies\tracy\vcpkg"
call "install_vcpkg_dependencies.bat"
if %ERRORLEVEL% GEQ 1 goto :ERROR
cd %XCORE_PATH%

"%MSBUILD%" "%CD%\..\dependencies\tracy\profiler\build\win32\Tracy.sln" /p:configuration=Release /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :ERROR

:DONE
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XCORE (BUILD TRACY) - SUCCESSFULLY DONE!!
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
goto :PAUSE

:ERROR
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------
powershell write-host -fore Red XCORE (BUILD TRACY) - FAILED!!
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------

:PAUSE
rem if no one give us any parameters then we will pause it at the end, else we are assuming that another batch file called us
if %1.==. pause