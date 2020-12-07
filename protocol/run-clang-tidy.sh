#!/bin/bash
# this can only be run if compile_commands.json exists:
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON) must be set in CMakeLists.txt
# this must be run after calling make within desktop (and server, if applicable)

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

# find machine type (necessary for pwd behavior)
isWindows=0
unameOut="$(uname -s)"
case "${unameOut}" in
    CYGWIN*)    isWindows=1;;
    MINGW*)     isWindows=1;;
esac

# array of all folders to be checked and modified
declare -a includeFolders=(
    "fractal"
    "desktop"
    "server"
)

yamlFolder="fixes"
mkdir $yamlFolder
fixesFilename=clang-tidy-fixes.yaml

# run clang-tidy and output suggested fixes to clang-tidy-fixes.yaml
echo "Running clang-tidy into ${yamlFolder}/${fixesFilename}"

# generate string of arguments to pass in for files to tidy
filesToFix="desktop/main.c server/main.c"
for folder in "${includeFolders[@]}"
do
    for cFilePath in $(find $folder -type f -regex ".*\.\c")
    do
        if [[ "$cFilePath" != *"desktop/main.c"* && "$cFilePath" != *"server/main.c"* ]]
        then
            filesToFix="${filesToFix} ${cFilePath}"
        fi
    done
done

# header files to be included in clang-tidy (don't want to include third-party folders)
headerFilter="desktop/|fractal/|server/"

clang-tidy -header-filter=$headerFilter --quiet --export-fixes=$yamlFolder/$fixesFilename $filesToFix

# ---- clean up yaml file before running replacements ----

# deletes all clang-diagnostic-error entries
echo "Deleting all clang-diagnostic-error entries"
yq d -i $yamlFolder/$fixesFilename 'Diagnostics.(DiagnosticName==clang-diagnostic-error)'

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

IFS=$'\n'
# normalizes all paths to get rid of '..' and make paths relative to current directory
echo "Normalizing paths"
for pathPair in $(yq r --printMode pv $yamlFolder/$fixesFilename 'Diagnostics.[*].**.FilePath')
do
    arr=( $(perl -E 'say for split quotemeta shift, shift' -- ": " "$pathPair" | tr -d \') )
    replacePath=$(realpath ${arr[1]})
    replacePath=${replacePath#"$thisDirectory"}
    yq w -i $yamlFolder/$fixesFilename ${arr[0]} $replacePath
done

# remove any diagnostic entries with excluded folder paths - some remain because of roundabout access (..)
echo "Removing excluded paths"
declare -a excludeFolders=(
    "include"
    "lib"
    "docs"
    "sentry-native"
    "share"
    "fractal/video/nvidia-linux"
)

for excludeFolder in "${excludeFolders[@]}"
do
    pathExpression="Diagnostics.(DiagnosticMessage.FilePath==${excludeFolder}/*)"
    yq d -i $yamlFolder/$fixesFilename $pathExpression
done

# array of specific files to exclude
declare -a excludeFiles=(
    "fractal/core/fractalgetopt.*"
)

for excludeFile in "${excludeFiles[@]}"
do
    pathExpression="Diagnostics.(DiagnosticMessage.FilePath==${excludeFile})"
    yq d -i $yamlFolder/$fixesFilename $pathExpression
done

if [[ $CICheck == 1 ]]
then
    numSuggestions=$(yq r -l ${yamlFolder}/${fixesFilename} Diagnostics)
    if [[ $numSuggestions != 0 ]]
    then
        echo "format issues found"
        exit 1
    fi
    echo "clang-tidy successful"
else
    echo "-----> CHECK PROPOSED REPLACEMENTS IN ${yamlFolder}/${fixesFilename} <-----"
    echo "----->       THEN TYPE 'c' TO REPLACE. ANY OTHER KEY WILL QUIT       <-----"

    read -n 1 -p "'c' to replace, ano other key to quit without replacing: " k
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
