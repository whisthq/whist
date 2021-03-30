#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

function printhelp {
    cat <<EOF
Usage: $0 [OPTION 1] [OPTION 2] ...

Note: Make sure to run this script in a terminal on macOS or Linux Ubuntu.

  --version VERSION         set the version number of the client app
                            must be greater than the current version
                            in S3 bucket

  --bucket BUCKET           set the S3 bucket to upload to (if -publish=true)
                            options are:
                              fractal-chromium-macos-dev      [macOS Fractalized Chrome Development Bucket]
                              fractal-chromium-macos-staging  [macOS Fractalized Chrome Staging Bucket]
                              fractal-chromium-macos-prod     [macOS Fractalized Chrome Production Bucket]
                              fractal-chromium-ubuntu-dev     [Linux Ubuntu Fractalized Chrome Development Bucket]
                              fractal-chromium-ubuntu-staging [Linux Ubuntu Fractalized Chrome Staging Bucket]
                              fractal-chromium-ubuntu-prod    [Linux Ubuntu Fractalized Chrome Production Bucket]

  --env ENV                 environment to publish the client app with
                            defaults to development, options are dev/staging/prod

  --publish PUBLISH         set whether to notarize the macOS application and publish to AWS S3 and auto-update live apps
                            defaults to false, options are true/false
EOF
}

if [[ "${1-}" == "--help" ]]; then
    printhelp
else
    version=${version:-1.0.0}
    bucket=${bucket:-PLACEHOLDER}
    env=${env:-development}
    publish=${publish:-false}

    # get named params
    while [ $# -gt 0 ]; do
        if [[ $1 == *"--"* ]]; then
            param="${1/--/}"
            if [[ -z "${2+x}" ]]; then
              echo "Could not find value for parameter $param"
              printhelp
              exit 1
            fi
            declare $param="$2"
        fi
        shift
    done

    python3 setVersion.gyp $bucket $version

    # Make FractalClient and create its app bundle
    cd ../../protocol
    mkdir -p build-publish
    cd build-publish
    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
    make -j FractalClient
    cd ..
    cd ../client-applications/desktop

    rm -rf loading 
    mkdir -p loading
    cp -r ../../protocol/build-publish/client/build64/loading/*.png loading

    rm -rf protocol-build
    mkdir -p protocol-build/client

    # Move FractalClient and related files over to client-app
    cp -r ../../protocol/build-publish/client/build64/* ./protocol-build/client
    rm -rf ./protocol-build/client/loading

    mv ./protocol-build/client/FractalClient "./protocol-build/client/ Fractal"


    # On Mac, codesign everything in ./protocol-build/client directory
    # Only notarize, when publish=true
    if [[ "$publish" == "false" ]]; then
        export CSC_IDENTITY_AUTO_DISCOVERY=false
    else 
        find ./protocol-build/client -exec codesign -f -v -s "Fractal Computers, Inc." {} \;
    fi

    # Initialize yarn first
    yarn -i

    # Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    yarn config set network-timeout 600000

    if [[ "$publish" == "true" ]]; then
        if [[ "$env" == "dev" ]]; then
            yarn set-prod-env-dev
        elif [[ "$env" == "staging" ]]; then
            yarn set-prod-env-staging
        elif [[ "$env" == "prod" ]]; then
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
