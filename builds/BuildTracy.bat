echo "This is not working corrently"
goto :PAUSE

@echo OFF
cd %CD%

SET MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MsBuild.exe

##set MSBUILD=
##for /D %%D in (%SYSTEMROOT%\Microsoft.NET\Framework\v4*) do set MSBUILD=%%D\MSBuild.exe

IF EXIST "%MSBUILD%" GOTO :PROCESS
:NOMSB
echo. 
echo MSBUILD not found 
echo. 
GOTO :PAUSE
:PROCESS

@echo ON
set "INCLUDE=..\..\..\..\freetype\include;..\..\..\..\glfw-3.3.bin.WIN64\include;%INCLUDE%"
set "LIB=..\..\..\..\freetype\win64;..\..\..\..\glfw-3.3.bin.WIN64\lib-vc2019;%LIB%"
REM use environment variables for INCLUDE and LIB values
##set UseEnv=true
"%MSBUILD%" "%CD%\..\dependencies\tracy\profiler\build\win32\tracy.sln" /t:build /p:configuration=Release /p:Platform="x64" /verbosity:minimal 
if %ERRORLEVEL% GEQ 1 goto :PAUSE

####

:PAUSE
pause 