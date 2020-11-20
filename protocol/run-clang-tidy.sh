#!/bin/bash
# this can only be run if compile_commands.json exists:
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON) must be set in CMakeLists.txt

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

IFS=$'\n'
# normalizes any paths that have '..'
echo "Normalizing paths"
for pathPair in $(yq r --printMode pv $yamlFolder/$fixesFilename 'Diagnostics.[*].**.FilePath')
do
    arr=( $(perl -E 'say for split quotemeta shift, shift' -- ": " "$pathPair" | tr -d \') )
    if [[ "${arr[1]}" == *".."* ]]
    then
        yq w -i $yamlFolder/$fixesFilename ${arr[0]} $(realpath ${arr[1]})
    fi
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
    pathExpression="Diagnostics.(DiagnosticMessage.FilePath==$(pwd)/${excludeFolder}/*)"
    yq d -i $yamlFolder/$fixesFilename $pathExpression
done

echo "-----> CHECK PROPOSED REPLACEMENTS IN ${yamlFolder}/${fixesFilename} <-----"
echo "----->       THEN TYPE 'c' TO REPLACE. ANY OTHER KEY WILL QUIT       <-----"

read -n 1 -p "'c' to replace, ano other key to quit without replacing: " k
if [[ $k = c ]]
then
    echo "Running clang-apply-replacements"
    # run clang-tidy noted replacements
    clang-apply-replacements $yamlFolder

    # cleanup
    rm -rf $yamlFolder
fi
