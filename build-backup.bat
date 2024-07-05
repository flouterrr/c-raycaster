cmake -S . -B .\build -G "Ninja" -Wdev
cmake --build .\build -v
@echo off
if "%~1"=="-r" goto Run
goto SkipRun
:Run
@echo on
.\build\tictactoe
@echo off
:SkipRun