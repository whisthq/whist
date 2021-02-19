@echo off

setlocal enabledelayedexpansion

set "map=%*"
call :gettoken -version version
call :gettoken -bucket bucket
call :gettoken -env env
call :gettoken -publish publish

if "%1%" == "--help" (
    call :printhelp
) else (
    echo Make sure to run this script in a x86_x64 terminal on Windows.

    PowerShell.exe -ExecutionPolicy Bypass -Command "& './setVersion.ps1'" -version %version% -bucket %bucket%

    cd ..\..\protocol
    cmake . -D CMAKE_BUILD_TYPE=Release -G "NMake Makefiles" -D BUILD_SERVER=OFF
    nmake FractalClient
    cd ..\client-applications\desktop
    rmdir /S/Q protocol-build
    mkdir protocol-build
    cd protocol-build
    mkdir desktop
    mkdir desktop\loading
    cd ..
    
    REM Rename FractalClient to Fractal for consistency with Electron app name, and move over to client-app
    rename ..\..\protocol\desktop\build64\Windows\FractalClient.exe Fractal.exe
    xcopy /s ..\..\protocol\desktop\build64\Windows protocol-build\desktop

    REM Note: we no longer add the logo to the executable because the logo gets set
    REM in the protocol directly via SDL

    REM initialize yarn first
    yarn -i

    REM Increase yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    yarn config set network-timeout 300000

    if "%publish%" == "true" (
        if "%env%" == "dev" (
            yarn set-prod-env-dev
        ) else (
            if "%env%" == "staging" (
                yarn set-prod-env-staging
            ) else (
                if "%env%" == "prod" (
                    yarn set-prod-env-prod
                ) else (
                    echo Did not set a valid environment; not publishing. Options are dev/staging/prod
                    exit 1
                )
            )
        )
        REM Package the application and upload to AWS S3 bucket
        yarn package-ci
    ) else (
        REM Package the application locally, without uploading to AWS S3 bucket
        yarn package
    )
)

goto :eof
:printhelp
echo Usage: build [OPTION 1] [OPTION 2] ...
echo.
echo Note: Make sure to run this script in a x86_x64 terminal on Windows.
echo Note: All arguments to both long and short options are mandatory.
echo.
echo   -version=VERSION              set the version number of the client app
echo                                 must be greater than the current version
echo                                 in S3 bucket
echo.
echo   -bucket=BUCKET                set the S3 bucket to upload to (if -publish=true)
echo                                 options are:
echo                                     fractal-chromium-windows-dev     [Windows Fractalized Chrome Development Bucket]
echo                                     fractal-chromium-windows-staging [Windows Fractalized Chrome Staging Bucket]
echo                                     fractal-chromium-windows-prod    [Windows Fractalized Chrome Production Bucket]
echo.
echo  -env ENV                       environment to publish the client app with
echo                                 defaults to development, options are dev/staging/prod
echo.
echo   -publish=PUBLISH              set whether to publish to S3 and auto-update live apps
echo                                 defaults to false, options are true/false
exit /b 0

:gettoken
call set "tmpvar=%%map:*%1=%%"
if "%tmpvar%"=="%map%" (set "%~2=") else (
for /f "tokens=1 delims= " %%a in ("%tmpvar%") do set tmpvar=%%a
set "%~2=!tmpvar:~1!"
)
exit /b
