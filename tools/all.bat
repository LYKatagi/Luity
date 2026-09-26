@echo off

make clean
if errorlevel 1 exit /b 1

make
if errorlevel 1 exit /b 1

make run
exit /b %errorlevel%
