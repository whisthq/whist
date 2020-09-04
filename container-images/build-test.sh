if [ -d $1/protocol-temp ]
then
   rm -rf $1/protocol-temp
fi
mkdir $1/protocol-temp
sudo cp -r $3/server/build64/* $1/protocol-temp
echo "Copied protocol over..."
sudo chmod +r $1/protocol-temp/*
docker build -f $1/Dockerfile.$2 $1 -t $1-systemd-$2
