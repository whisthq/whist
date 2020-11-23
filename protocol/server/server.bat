@echo off

REM Bypass "Terminate Batch Job" prompt.
if "%~1"=="-FIXED_CTRL_C" (
   REM Remove the -FIXED_CTRL_C parameter
   SHIFT
) ELSE (
   REM Run the batch with <NUL and -FIXED_CTRL_C
   CALL <NUL %0 -FIXED_CTRL_C %*
   GOTO :EOF
)

SET RESTVAR=
:loop1
IF "%1"=="" GOTO after_loop
SET RESTVAR=%RESTVAR% %1
SHIFT
GOTO loop1

:after_loop

cd build64
FractalServer %RESTVAR%
cd ..
