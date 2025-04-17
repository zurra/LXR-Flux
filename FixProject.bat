@echo off
setlocal
echo -- Deleting Binaries, Intermediate, Build --
cd %~dp0
rmdir /s /q "%~dp0Binaries" 2>nul
rmdir /s /q "%~dp0Intermediate" 2>nul
rmdir /s /q "%~dp0Build" 2>nul
call "%~dp0GenerateProjectFiles.bat"
"%p%"
exit