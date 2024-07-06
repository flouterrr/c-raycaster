@echo off
set "proj_name=raycaster"
set "compiler=gcc"
set "generator=Ninja"
goto Start

:Start
@echo off
if "%~1"=="-d" goto SetDebug
if "%~2"=="-d" goto SetDebug
set "cmake_build_type=Release"
goto Configure

:SetDebug
set "cmake_build_type=Debug"
goto Configure

:Configure
@echo on
cmake -S . -B .\build\%cmake_build_type% -G"%generator%" -Wdev -DCMAKE_C_COMPILER=%compiler% -DCMAKE_BUILD_TYPE=%cmake_build_type%
copy .\build\%cmake_build_type%\compile_commands.json . /y
@echo off
goto Build

:Build
@echo on
cmake --build .\build\%cmake_build_type% -v
@echo off
goto RunCheck

:RunCheck
if "%~1"=="-r" goto Run
if "%~2"=="-r" goto Run
goto SkipRun

:Run
@echo on
.\build\%cmake_build_type%\%proj_name%
@echo off

:SkipRun