@echo off
setlocal
for /r "%~dp0" %%a in (*.uproject) do set p=%%~dpnxa
powershell -command "& { (Get-ItemProperty 'Registry::HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj' -Name 'Icon' ).'Icon' }" > gen_temp.txt
set /p gen_bin=<gen_temp.txt
echo -- Generating project files --
%gen_bin% /projectfiles "%p%"
del /q gen_temp.txt
