#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Use cwd as the build directory
BUILD_DIR="$(pwd)"

# cd into where run-clang-tidy.sh is (I.e., cd into the source directory)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# This script runs clang-tidy to check for C coding standards and linting, and propose
# edits if it finds areas for improvements.

# This script can only be run if compile_commands.json exists:
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON) must be set in CMakeLists.txt
# This script can only be run after calling make within /client (and /server, if applicable)

# run with option -c for CI check without replacement option
OPTIND=1
CICheck=

while getopts "c" opt; do
  case "$opt" in
    c) CICheck=1 ;;
    *) echo "Invalid Option: $opt" ;;
  esac
done

shift $((OPTIND-1))

[ "${1:-}" = "--" ] && shift

# find machine type (necessary for appropriate pwd behavior)
isWindows=
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
yamlFolder="$BUILD_DIR/fixes"
rm -rf "$yamlFolder" # -f to silence "no file found" error
mkdir "$yamlFolder"
fixesFilename=clang-tidy-fixes.yaml
fixesFile="$yamlFolder/$fixesFilename"

# run clang-tidy and output suggested fixes to clang-tidy-fixes.yaml
echo "Running clang-tidy into $fixesFile"

FILES_EXCLUDE="include|lib|docs|sentry-native|share|nvidia-linux|fractalgetopt\.[ch]|lodepng\.[ch]"

# Generate list of files to pass into clang-tidy
# We parse compile_commands ourselves, instead of just listing all of our .c files,
# because new LLVM versions on Mac/Windows fail to parse compile_commands.json correctly
# This causes it to not filter .c files from other OS's, which causes errors.
compileCommands="$BUILD_DIR/compile_commands.json"
if [[ ! -f "$compileCommands" ]]; then
  echo "Hm, $compileCommands not found."
  echo "Note that you must call $0 _from_ the build directory that you called \"make\" in"
  exit 1
fi
filesToFix=()
IFS=$'\n' # Break for-loop on newline rather than any whitespace
for line in $(cat "$compileCommands" | jq -r '.[].file' | grep -Ev "($FILES_EXCLUDE)"); do
  if [[ -n "$isWindows" ]]; then
    # Replace Windows path with Linux path
    line="$(echo "$line" | sed "s/\([^:]*\):/\/\1/g")"
  fi
  filesToFix+=("$line")
done
unset IFS

# Replace /experimental:external /external:W0 /external:I with -I, since clang can only read -I
if [[ -n "$isWindows" ]]; then
  sed -i 's/\/experimental:external//g' "$compileCommands"
  sed -i 's/\/external:W0//g' "$compileCommands"
  sed -i 's/\/external:I/-I/g' "$compileCommands"
fi

# header files to be included in clang-tidy (we don't want to include third-party headers, only our code)
headerFilter="^((?!$FILES_EXCLUDE).)*$"

# If clang-tidy succeeded and it didn't generate a fixesFile, then we're good to go
# (clang-tidy will return error code 0 even if it puts warnings in the fixesFile)
if [[ ! -f ".clang-tidy" ]]; then
  echo "Hm, .clang-tidy not found, is run-clang-tidy.sh not in the source directory?"
  exit 1
fi
if clang-tidy -p="$BUILD_DIR" --header-filter="$headerFilter" --quiet --export-fixes="$fixesFile" "${filesToFix[@]}" && [[ ! -f "$fixesFile" ]]; then
  echo "clang-tidy successful"
else
  if [[ -n "$CICheck" ]]; then
    # A yaml file with no format issues should have exactly 4 lines
    echo "Format issues found. See $fixesFile"
    exit 1
  else
    echo "-----> CHECK PROPOSED REPLACEMENTS IN ${fixesFile} <-----"
    echo "----->       THEN TYPE 'r' TO REPLACE ALL OF THEM. ANY OTHER KEY WILL QUIT       <-----"

    read -r -n 1 -p "'r' to replace, any other key to quit without replacing: " k
    if [[ "$k" == "r" ]]; then
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
fi

# cleanup
rm -r "$yamlFolder"
