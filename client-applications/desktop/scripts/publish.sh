#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

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

if [[ "${1-}" == "--help" ]]
then
    printhelp
else
    version=${version:-1.0.0}
    bucket=${bucket:-PLACEHOLDER}
    env=${env:-dev}
    publish=${publish:-false}

    # get named params
    while [[ "$#" -gt 0 ]]; do
        if [[ "$1" == *"--"* ]]; then
            param="${1/--/}"
            if [[ ! "$param" =~ (version|bucket|env|publish) ]]; then
                echo "Error: $param is not a valid parameter"
                printhelp
                exit 1
            fi
            if [[ -z "${2+x}" ]]; then
                echo "Error: Could not find value for parameter $param"
                printhelp
                exit 1
            fi
            declare $param="$2"
        else
          echo "Error: $1 is not a valid parameter"
          printhelp
          exit 1
        fi
        shift
        shift
    done

    export BUILD_NUMBER=$version

    # Make FractalClient and create its app bundle
    cd ../../../protocol
    mkdir -p build-publish
    cd build-publish
    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
    make -j FractalClient
    cd ../../client-applications/desktop

    # Create protocol-build directory
    rm -rf protocol-build
    mkdir protocol-build
    cd protocol-build
    mkdir client
    cd ..

    # Move FractalClient and related files over to client-app build folder
    cp -r ../../../protocol/build-publish/client/build64/* ./protocol-build/client
    # Move loading png's out because Electron builder doesn't like them
    mv ./protocol-build/client/loading ./loading
    # Rename FractalClient to Fractal
    mv ./protocol-build/client/FractalClient ./protocol-build/client/Fractal

    # Codesign if publishing, or don't codesign at all if not publishing
    if [[ "$publish" == "false" ]]; then
        export CSC_IDENTITY_AUTO_DISCOVERY=false
    else
        # On Mac,
        if [[ "$(uname -s)" == "Darwin" ]]; then
            # codesign everything in protocol-build/client
            find protocol-build/client -type f -exec codesign -f -v -s "Fractal Computers, Inc." {} \;
        fi
    fi

    # Initialize yarn first
    yarn -i

    # Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    yarn config set network-timeout 600000

    if [[ "$publish" == "true" ]]; then
        if [[ "$env" == "dev" ]]; then
            node ./scripts/setProdEnv.js dev
        elif [[ "$env" == "staging" ]]; then
            node ./scripts/setProdEnv.js staging
        elif [[ "$env" == "prod" ]]; then
            node ./scripts/setProdEnv.js prod
        else
            echo "Did not set a valid --env environment; not publishing. Options are dev/staging/prod"
            exit 1
        fi

        # Package the application and upload to AWS S3 bucket
        export S3_BUCKET=$bucket && yarn package-ci
    else
        # Package the application locally, without uploading to AWS S3 bucket
        export S3_BUCKET=$bucket && yarn package
    fi
fi
