@echo off

REM cd to parent directory of this script
cd "%~dp0"

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

REM We use the ~dp0 syntax to put our logs in the parent directory
REM of this script. Note that we must cd to client\build64 for
REM the FractalClient working directory to include DLLs and loading
REM PNGs properly.

cd client\build64
echo "Running client protocol silently. Logs are in client.log..."
FractalClient %RESTVAR% > "%~dp0"\client.log
cd ..\..
