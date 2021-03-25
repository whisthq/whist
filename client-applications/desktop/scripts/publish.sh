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

if [[ "${1-default}" == "--help" ]]
then
    printhelp
else
    version=${version:-1.0.0}
    bucket=${bucket:-PLACEHOLDER}
    env=${env:-dev}
    publish=${publish:-false}

    # get named params
    while [ $# -gt 0 ]; do
        if [[ $1 == *"--"* ]]; then
            param="${1/--/}"
            declare $param="$2"
        fi
        shift
    done
    
    export BUILD_NUMBER=$version

    # Make FractalClient and create its app bundle
    cd ../../protocol
    cmake . -DCMAKE_BUILD_TYPE=Release
    make FractalClient
    cd ../client-applications/desktop
    rm -rf protocol-build
    mkdir -p protocol-build/client

    rm -rf loading 
    mkdir -p loading

    # Move FractalClient and crashpad_handler over to client-app
    cp ../../protocol/client/build64/Darwin/FractalClient protocol-build/client/Fractal
    cp ../../protocol/client/build64/Darwin/crashpad_handler protocol-build/client/crashpad_handler

    # Copy over the FFmpeg dylibs
    cp ../../protocol/lib/64/ffmpeg/Darwin/*.dylib protocol-build/client
    cp ../../protocol/client/build64/Darwin/*.dylib protocol-build/client

    # Copy loading images to a temp folder (will be moved in afterSign script)
    cp -r ../../protocol/client/build64/Darwin/loading/*.png loading

    # Codesign if publishing, or don't codesign at all if not publishing
    if [[ "$publish" == "false" ]]; then
        export CSC_IDENTITY_AUTO_DISCOVERY=false
    else
        find protocol-build/client -exec codesign -f -v -s "Fractal Computers, Inc." {} \;
    fi

    # Initialize yarn first
    yarn -i

    # Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    yarn config set network-timeout 600000

    if [[ "$publish" == "true" ]]
    then
        if [[ "$env" == "dev" ]]
        then
            node ./scripts/setProdEnv.js dev
    elif [[ "$env" == "staging" ]]
        then
            node ./scripts/setProdEnv.js staging
    elif [[ "$env" == "prod" ]]
        then
            node ./scripts/setProdEnv.js prod
        else
            echo "Did not set a valid environment; not publishing. Options are dev/staging/prod"
            exit 1
        fi        

        # Package the application and upload to AWS S3 bucket
        export S3_BUCKET=$bucket && yarn package-ci
    else
        # Package the application locally, without uploading to AWS S3 bucket
        yarn package
    fi
fi