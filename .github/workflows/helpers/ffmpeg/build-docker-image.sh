docker build -f Dockerfile.$1 --build-arg FFmpegrepo=. . -t ffmpeg-builder-ubuntu$1
