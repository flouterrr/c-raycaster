@echo off
set "proj_name=raycaster"
goto Start

:Start
goto FreshCheck

:FreshCheck
if "%~1"=="-f" goto Fresh
if "%~2"=="-f" goto Fresh
goto NotFresh

:Fresh
@echo on
cmake -S . -B .\build -G "Ninja" -Wdev -DCMAKE_C_COMPILER=gcc --fresh
@echo off
goto Build

:NotFresh
@echo on
cmake -S . -B .\build -G "Ninja" -Wdev -DCMAKE_C_COMPILER=gcc
@echo off
goto Build

:Build
@echo on
cmake --build .\build -v
@echo off
goto RunCheck

:RunCheck
if "%~1"=="-r" goto Run
if "%~2"=="-r" goto Run
goto SkipRun
:Run
@echo on
.\build\%proj_name%
@echo off
:SkipRun