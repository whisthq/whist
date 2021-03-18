#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

function printhelp {
    echo "Usage: build [OPTION 1] [OPTION 2] ...\n"
    echo "Note: Make sure to run this script in a terminal on macOS or Linux Ubuntu."
    echo ""
    echo "  --version VERSION         set the version number of the client app"
    echo "                            must be greater than the current version"
    echo "                            in S3 bucket"
    echo ""
    echo "  --bucket BUCKET           set the S3 bucket to upload to (if -publish=true)"
    echo "                            options are:"
    echo "                              fractal-chromium-macos-dev      [macOS Fractalized Chrome Development Bucket]"
    echo "                              fractal-chromium-macos-staging  [macOS Fractalized Chrome Staging Bucket]"
    echo "                              fractal-chromium-macos-prod     [macOS Fractalized Chrome Production Bucket]"
    echo "                              fractal-chromium-ubuntu-dev     [Linux Ubuntu Fractalized Chrome Development Bucket]"
    echo "                              fractal-chromium-ubuntu-staging [Linux Ubuntu Fractalized Chrome Staging Bucket]"
    echo "                              fractal-chromium-ubuntu-prod    [Linux Ubuntu Fractalized Chrome Production Bucket]"
    echo ""
    echo "  --env ENV                 environment to publish the client app with"
    echo "                            defaults to development, options are dev/staging/prod"
    echo ""
    echo "  --publish PUBLISH         set whether to notarize the macOS application and publish to AWS S3 and auto-update live apps"
    echo "                            defaults to false, options are true/false"
}

if [[ "$1" == "--help" ]]
then
    printhelp
else
    version=${version:-1.0.0}
    bucket=${bucket:-fractal-chromium-macos-dev}
    env=${env:-development}
    publish=${publish:-false}

    # get named params
    while [ $# -gt 0 ]; do
        if [[ $1 == *"--"* ]]; then
            param="${1/--/}"
            declare $param="$2"
        fi
        shift
    done

    # Only notarize, when publish=true
    if [[ "$publish" == "false" ]]; then
        export CSC_IDENTITY_AUTO_DISCOVERY=false
    fi
    python3 setVersion.gyp $bucket $version

    # Make FractalClient and create its app bundle
    cd ../../protocol
    cmake . -DCMAKE_BUILD_TYPE=Release
    make FractalClient
    cd ../client-applications/desktop
    rm -rf protocol-build
    mkdir -p protocol-build/client

    # Move FractalClient and crashpad_handler over to client-app
    cp ../../protocol/client/build64/Darwin/FractalClient protocol-build/client/FractalClient
    cp ../../protocol/client/build64/Darwin/crashpad_handler protocol-build/client/crashpad_handler

    # Copy over the FFmpeg dylibs
    cp ../../protocol/lib/64/ffmpeg/Darwin/*.dylib protocol-build/client
    cp ../../protocol/client/build64/Darwin/*.dylib protocol-build/client

    # Sign each FractalClient binary
    for filename in protocol-build/client/*.dylib; do
        codesign -f -v -s "Fractal Computers, Inc." $filename
    done

    codesign -f -v -s "Fractal Computers, Inc." protocol-build/client/crashpad_handler
    codesign -f -v -s "Fractal Computers, Inc." protocol-build/client/FractalClient

    # Copy loading images to a temp folder (will be moved in afterSign script)
    rm -rf loadingtemp
    cp -r ../../protocol/client/build64/Darwin/loading loadingtemp
    for filename in loadingtemp/*; do
        codesign -f -v -s "Fractal Computers, Inc." $filename
    done

    # Initialize yarn first
    yarn -i

    # Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    yarn config set network-timeout 300000

    if [[ "$publish" == "true" ]]
    then
        if [[ "$env" == "dev" ]]
        then
            yarn set-prod-env-dev
    elif [[ "$env" == "staging" ]]
        then
            yarn set-prod-env-staging
    elif [[ "$env" == "prod" ]]
        then
            yarn set-prod-env-prod
        else
            echo "Did not set a valid environment; not publishing. Options are dev/staging/prod"
            exit 1
        fi
        # Package the application and upload to AWS S3 bucket
        yarn package-ci
    else
        # Package the application locally, without uploading to AWS S3 bucket
        yarn package
    fi
fi
