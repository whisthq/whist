declare -a excludeFolders=(
    "include"
    "lib"
    "fractal/video/nvidia-linux"
)

fileFilterString="clang-tidy --header-filter=\".*\" --line-filter='["

for folder in "${excludeFolders[@]}"
do
    for filePath in $(find $folder -type f -print)
    do
        fileFilterString="${fileFilterString}{\"name\":\"${filePath}\",\"lines\":[[9999999,9999999]]}, "
    done
done

fileFilterString="${fileFilterString},{\"name\":\"c\"}, {\"name\":\"h\"}]'"

echo $fileFilterString

