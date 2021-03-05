#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# This script runs clang-tidy to check for C coding standards and linting, and propose
# edits if it finds areas for improvements.

# This script can only be run if compile_commands.json exists:
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON) must be set in CMakeLists.txt
# This script can only be run after calling make within /client (and /server, if applicable)

# run with option -c for CI check without replacement option
OPTIND=1
CICheck=0

while getopts "c" opt
do
    case "$opt" in
        c)
            CICheck=1
    esac
done

shift $((OPTIND-1))

[ "${1:-}" = "--" ] && shift

# find machine type (necessary for appropriate pwd behavior)
isWindows=0
unameOut="$(uname -s)"
case "${unameOut}" in
    CYGWIN*)    isWindows=1 ;;
    MINGW*)     isWindows=1 ;;
esac

# array of all folders to be checked and modified
declare -a includeFolders=(
    "fractal"
    "client"
    "server"
)

# set clang-tidy-fixes file
yamlFolder="fixes"
mkdir $yamlFolder
fixesFilename=clang-tidy-fixes.yaml

# run clang-tidy and output suggested fixes to clang-tidy-fixes.yaml
echo "Running clang-tidy into ${yamlFolder}/${fixesFilename}"

# generate string of arguments to pass in for files to tidy
filesToFix="client/main.c server/main.c"
for folder in "${includeFolders[@]}"
do
    for cFilePath in $(find "$folder" -type f -regex ".*\.\c")
    do
        if [[ "$cFilePath" != *"client/main.c"* && "$cFilePath" != *"server/main.c"* ]]
        then
            filesToFix="${filesToFix} ${cFilePath}"
        fi
    done
done

# header files to be included in clang-tidy (we don't want to include third-party folders, only our code)
headerFilter="client/|fractal/|server/"

clang-tidy --header-filter=$headerFilter --quiet --export-fixes=$yamlFolder/$fixesFilename $filesToFix || true

# ---- clean up yaml file before running replacements ----

# deletes all clang-diagnostic-error entries
perl -i -p -000 -e 's/  - DiagnosticName:[ ]*(clang-diagnostic-error|clang-diagnostic-unknown-pragmas)[^\n]*\n(    [^\n]*\n)*//g' ${yamlFolder}/${fixesFilename}

# get current directory path based on OS
if [[ $isWindows == 1 ]]
then
    thisDirectory=$(cmd '/C echo %cd%
    ')
    thisDirectory=$(realpath "$thisDirectory")
else
    thisDirectory=$(pwd)
fi
thisDirectory="${thisDirectory}/"

# remove any diagnostic entries with excluded folder paths - some remain because of roundabout access (..)
perl -i -p -000 -e 's/  - DiagnosticName:[^\n]*\n(    [^\n]*\n)*[ ]*FilePath:[ ]*'"'"'?[:\/\\\w\.-]*(include|lib|docs|sentry-native|share|nvidia-linux|fractalgetopt\.[ch]|lodepng\.[ch])[:\/\\\w\.]*'"'"'?\n(    [^\n]*(\n|$))*//g' ${yamlFolder}/${fixesFilename}

if [[ $CICheck == 1 ]]
then
    numLines=$(cat ${yamlFolder}/${fixesFilename} | wc -l | tr -d ' ') # wc on mac has leading whitespace for god knows what reason
    # A yaml file with no format issues should have exactly 4 lines
    if [[ $numLines != 4 ]]
    then
        echo "Format issues found. See ${yamlFolder}/${fixesFilename}"
        exit 1
    fi
    echo "clang-tidy successful"
else
    echo "-----> CHECK PROPOSED REPLACEMENTS IN ${yamlFolder}/${fixesFilename} <-----"
    echo "----->       THEN TYPE 'c' TO REPLACE. ANY OTHER KEY WILL QUIT       <-----"

    read -r -n 1 -p "'c' to replace, any other key to quit without replacing: " k
    if [[ $k == c ]]
    then
        echo
        echo "Running clang-apply-replacements"

        # run clang-tidy noted replacements
        if command -v clang-apply-replacements &> /dev/null
        then
            clang-apply-replacements $yamlFolder
        else
            clang-apply-replacements-10 $yamlFolder
        fi
    else
        exit
    fi
fi

# cleanup
rm -rf $yamlFolder
