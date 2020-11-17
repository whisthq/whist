#!/bin/bash
# this can only be run if compile_commands.json exists:
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON) must be set in CMakeLists.txt

# array of all folders to be ignored (some paths are roundabout accessed)
declare -a excludeFolders=(
    "include"
    "include/ffmpeg/libavcodec/../libavutil/../.."
    "include/ffmpeg/libavformat/../libavcodec/../libavutil/../.."
    "lib"
    "fractal/video/nvidia-linux"
)

# array of all folders to be checked and modified
declare -a includeFolders=(
    "fractal"
    "desktop"
)

# iterate through exclusion folders and option to only read the 9999999th line of each of these files
fileFilterString="--line-filter=["
for folder in "${excludeFolders[@]}"
do
    for filePath in $(find $folder -type f -print)
    do
        fileFilterString="${fileFilterString}{\"name\":\"${filePath}\",\"lines\":[[9999999,9999999]]}, "
    done
done

# include the rest of the .c and .h files
fileFilterString="${fileFilterString} {\"name\":\"c\"}, {\"name\":\"h\"}]"

yamlFolder="fixes"
mkdir $yamlFolder

# echo $fileFilterString

filesToFix="desktop/main.c server/main.c"

for folder in "${includeFolders[@]}"
do
    for cFilePath in $(find $folder -type f -regex ".*\.\c")
    do
        if [[ "$cFilePath" != *"desktop/main.c"* ]]
        then
            filesToFix="${filesToFix} ${cFilePath}"
        fi
        # # run clang-tidy and output to clang-tidy-fixes.yaml
        # clang-tidy -header-filter=.* "${fileFilterString}" $cFilePath --export-fixes=${yamlFolder}/clang-tidy-fixes.yaml

        # # run clang-tidy noted replacements
        # clang-apply-replacements $yamlFolder

        # sleep 1
    done
done

# run clang-tidy and output suggested fixes to clang-tidy-fixes.yaml
clang-tidy -header-filter=.* "${fileFilterString}" --quiet --export-fixes=${yamlFolder}/clang-tidy-fixes.yaml $filesToFix

# old .clang-tidy
# Checks: '-*,readability-identifier-naming'
# CheckOptions: [
#     {key: readability-identifier-naming.FunctionCase, value: lower_case},
#     {key: readability-identifier-naming.VariableCase, value: lower_case}
# ]

# cleanup
# rm -rf $yamlFolder