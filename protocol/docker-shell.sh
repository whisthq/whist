#!/bin/bash
# ./builder/build_image.sh $1
# abs_repo_path=$(cd $1;pwd)
# echo "Repo abs path is $abs_repo_path"
container_name=protocol-builder-$(date +"%s")
docker run -it  --mount type=bind,source=$(pwd),destination=/workdir --name $container_name \
            --user $(id -u ${USER}) protocol-builder-ubuntu$1 

# docker container kill $container_name