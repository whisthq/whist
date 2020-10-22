#!/bin/bash
echo “Running userdata-bootstrap.sh”

aws s3 cp s3://fractal-ecs-host-service/ecs-host-service ecs-host-service
chmod +x ecs-host-service


# Write systemd unit file for ECS Host Service
sudo cat << EOF > /etc/systemd/system/ecs-host-service.service
[Unit]
Description=ECS Host Service
Requires=docker.service
After=docker.service

[Service]
Restart=always
User=root
Type=exec
ExecStart=/home/ubuntu/ecs-host-service

[Install]
WantedBy=multi-user.target
EOF

# Reload daemon files
sudo /bin/systemctl daemon-reload

# Disabling ECS Host Service (see README)
sudo systemctl disable ecs-host-service.service
