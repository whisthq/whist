@echo off

setlocal enabledelayedexpansion

set "map=%*"
call :gettoken -branch branch 
call :gettoken -version version 
call :gettoken -bucket bucket 
call :gettoken -publish publish 

if "%1%" == "--help" (
    call :printhelp
) else (
    echo Make sure to run this script in a x86_x64 terminal on Windows.

    PowerShell.exe -ExecutionPolicy Bypass -Command "& './setVersion.ps1'" -version %version% -bucket %bucket%

    rmdir /S/Q .protocol
    git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
    cd .protocol
    git reset --hard
    git fetch --depth 25 origin %branch%:%branch%
    git checkout %branch%
    cmake -G "NMake Makefiles"
    nmake FractalClient
    cd ..
    rmdir /S/Q protocol
    mkdir protocol
    cd protocol
    mkdir desktop
    mkdir desktop\loading
    cd ..
    xcopy /s .protocol\desktop\build64\Windows protocol\desktop
    powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe', 'rcedit-x64.exe')"
    powershell -Command "Invoke-WebRequest https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe -OutFile rcedit-x64.exe"
    rcedit-x64.exe protocol\desktop\FractalClient.exe --set-icon build\icon.ico
    del /Q rcedit-x64.exe

    if "%publish%" == "true" (
        yarn package-ci 
        REM yarn package-ci && curl -X POST -H Content-Type:application/json  -d "{ \"branch\" : \"%1\", \"version\" : \"%2\" }" https://cube-celery-staging2.herokuapp.com/version
    ) else (
        yarn package
    )
)

goto :eof
:printhelp
echo Usage: build [OPTION 1] [OPTION 2] ...
echo.
echo Note: Make sure to run this script in a x86_x64 terminal on Windows.
echo Note: All arguments to both long and short options are mandatory.
echo   -branch=BRANCH                set the Github protocol branch that you
echo                                   want the client app to run
echo   -version=VERSION              set the version number of the client app
echo                                   must be greater than the current version
echo                                   in S3 bucket
echo   -bucket=BUCKET                set the S3 bucket to upload to (if -publish=true)
echo                                   options are:
echo                                     fractal-applications-release [Windows bucket]
echo                                     fractal-mac-application-release [Mac bucket]
echo                                     fractal-linux-application-release [Linux bucket]
echo                                     fractal-applications-testing [Internal use testing bucket]
echo   -publish=PUBLISH              set whether to publish to S3 and auto-update live apps
echo                                   defaults to false, options are true/false
exit /b 0

:gettoken
call set "tmpvar=%%map:*%1=%%"
if "%tmpvar%"=="%map%" (set "%~2=") else (
for /f "tokens=1 delims= " %%a in ("%tmpvar%") do set tmpvar=%%a
set "%~2=!tmpvar:~1!"
)
exit /b