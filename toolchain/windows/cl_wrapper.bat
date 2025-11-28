@echo off

@REM set PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Users\root\AppData\Local\Python\pythoncore-3.14-64
set PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;

set VSLANG=1033
chcp 65001 >NUL 2>&1

if not exist _tmp mkdir _tmp
set TEMP=%cd%\_tmp
set TMP=%cd%\_tmp

@REM python "%~dp0cl_wrapper.py" %*
%~dp0cl_wrapper.exe %*