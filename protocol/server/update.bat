@ECHO OFF

SET CMD=%~dpn0

SET RESTVAR=
:loop1
IF "%1"=="" GOTO after_loop
SET RESTVAR=%RESTVAR%%1
SHIFT
GOTO loop1
:after_loop

powershell.exe -Command "& %CMD%.ps1 -Branch '%RESTVAR%'"
