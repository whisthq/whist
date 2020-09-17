# build protocol
base/protocol/docker-build-image.sh $2
base/protocol/docker-build-protocol.sh $2

# build container with protocol inside it
docker build -f $1/Dockerfile.$2 $1 -t fractal-$1-$2
