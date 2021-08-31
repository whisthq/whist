#!/bin/bash

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# If we run this script as root, then the "ubuntu"/default user will not be
# added to the "docker" group, only root will.
if [ "$EUID" -eq 0 ]; then
    echo "This script cannot be run as root!"
    exit
fi


# `usage` is the help function we provide.
usage () {
    cat << EOF
USAGE:
  setup_host.sh --localdevelopment
  setup_host.sh --deployment [ARGS...]
This script takes a blank Ubuntu 21.04 EC2 instance and sets it up for running
Fractal by configuring Docker and NVIDIA to run Fractal's GPU-enabled
mandelboxes.
To set up the host for local development, pass in the --localdevelopment flag.
To set it up for deployments, pass in the --deployment flag, followed by the
arguments described in the comment for \`deployment_setup_steps\`.
EOF

    # We set a nonzero exit code so that CI doesn't accidentally only run `usage` and think it succeeded.
    exit 2
}


# `common_steps` contains the commands that must be run for
# both the local and deploy environments. This was formerly
# known as `setup_ubuntu20_host.sh`. This function runs first in both environments.
#
# Args: none
common_steps () {
    cd "$DIR"

    # Set dkpg frontend as non-interactive to avoid irrelevant warnings
    echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections
    sudo apt-get install -y -q

    echo "================================================"
    echo "Replacing potentially outdated Docker runtime..."
    echo "================================================"

    # Attempt to remove potentially outdated Docker runtime
    # Allow failure with ||:, in case they're not installed yet
    sudo apt-get remove -y docker docker-engine docker.io containerd runc ||:
    sudo apt-get clean -y
    sudo apt-get upgrade -y
    sudo apt-get update -y

    # Install latest Docker runtime and dependencies
    sudo apt-get install -y apt-transport-https ca-certificates curl wget gnupg-agent software-properties-common
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
    sudo apt-key fingerprint 0EBFCD88
    sudo add-apt-repository -y \
        "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
       $(lsb_release -cs) \
        stable"
    sudo apt-get update -y
    sudo apt-get install -y docker-ce docker-ce-cli containerd.io

    # Attempt to Add Docker group, but allow failure with "||:" in case it already exists
    sudo groupadd docker ||:
    sudo gpasswd -a "$USER" docker

    # Always enable docker buildkit
    echo 'export DOCKER_BUILDKIT=1' | sudo tee /etc/profile.d/03-docker-buildkit

    echo "================================================"
    echo "Installing AWS CLI..."
    echo "================================================"

    # We don't need to configure the AWS CLI (only install it) because this script runs
    # on an AWS EC2 instance, which have awscli automatically configured
    sudo apt install -y awscli

    echo "================================================"
    echo "Installing NVIDIA drivers..."
    echo "================================================"

    # Stop any running nvidia-persistenced service
    sudo systemctl stop nvidia-persistenced.service ||:

    # Install Linux headers
    sudo apt-get install -y gcc make "linux-headers-$(uname -r)"

    # Blacklist some Linux kernel modules that would block NVIDIA drivers
    cat << EOF | sudo tee --append /etc/modprobe.d/blacklist.conf
blacklist vga16fb
blacklist nouveau
blacklist rivafb
blacklist nvidiafb
blacklist rivatv
EOF
    sudo sed -i 's/GRUB_CMDLINE_LINUX=""/# GRUB_CMDLINE_LINUX=""/g' /etc/default/grub
    cat << EOF | sudo tee --append /etc/default/grub
GRUB_CMDLINE_LINUX="rdblacklist=nouveau"
EOF

    # Install NVIDIA GRID (virtualized GPU) drivers
    ./get-nvidia-driver-installer.sh
    sudo chmod +x nvidia-driver-installer.run
    sudo ./nvidia-driver-installer.run --silent
    sudo rm nvidia-driver-installer.run

    echo "================================================"
    echo "Installing nvidia-docker..."
    echo "Note that (as of 10/5/20) the URLs may still say Ubuntu 18.04. This is because"
    echo "NVIDIA has redirected the corresponding Ubuntu 20.04 URLs to the 18.04 versions."
    echo "================================================"

    # Source nvidia-docker apt package
    # Note that we hardcode `distribution` to 20.04, so that we can upgrade to higher eventually,
    # unofficially supported Ubuntu versions eventully (i.e. 21.04) since the 20.04 install
    # (which just redirects to the 18.04 install) works for 21.04.
    distribution="ubuntu20.04"
    curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
    curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

    # Install nvidia-docker via apt
    sudo apt-get update -y
    sudo apt-get install -y nvidia-docker2

    echo "================================================"
    echo "Installing General Utilities..."
    echo "================================================"

    sudo apt-get install -y lsof jq tar lz4

    echo "================================================"
    echo "Configuring Docker daemon..."
    echo "================================================"

    # Set custom seccomp filter
    sudo systemctl start docker
    ./docker-daemon-config/generate-seccomp.sh

    sudo cp docker-daemon-config/daemon.json /etc/docker/daemon.json
    sudo cp docker-daemon-config/seccomp.json /etc/docker/seccomp.json

    sudo systemctl restart docker

    echo "================================================"
    echo "Installing Other Host Service Dependencies..."
    echo "================================================"

    sudo apt-get install -y openssl

    echo "================================================"
    echo "Installing Uinput Config Files..."
    echo "================================================"

    sudo cp fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules

    echo "================================================"
    echo "Installing monitoring services.."
    echo "================================================"
    # Filebeat(https://www.elastic.co/beats/filebeat) is a service that we install on hosts to ship logs from systemd services running inside mandelboxes to logz.io.
    # When host service creates a mandelbox, it mounts the logs generated by the mandelbox under a folder that is monitored by filebeat. Filebeat will parse the logs
    # written and send them out to our logz account as configured in `host-setup/filebeat-config/filebeat.yml`.
    # Post installation, by default filebeat doesn't startup on system boot/install.
    # The default behaviour is not modified in this file intentionally, to not ship logs from local-dev.
    # For local-dev, we want you to take a call on sending the logs to logz (if that workflow is going to be easier for you).
    # If you want to enable it, pleast take a look at `setup_ubuntu20_ami_host.sh` for further instructions.

    wget -qO - https://artifacts.elastic.co/GPG-KEY-elasticsearch | sudo apt-key add -
    echo "deb https://artifacts.elastic.co/packages/7.x/apt stable main" | sudo tee -a /etc/apt/sources.list.d/elastic-7.x.list

    # Install filebeat via apt
    sudo apt-get update -y
    sudo apt-get install -y filebeat

    # For HTTPS shipping, download the Logz.io public certificate to your certificate authority folder.
    # https://app.logz.io/#/dashboard/send-your-data/log-sources/filebeat
    sudo curl https://raw.githubusercontent.com/logzio/public-certificates/master/TrustExternalCARoot_and_USERTrustRSAAAACA.crt --create-dirs -o /etc/pki/tls/certs/COMODORSADomainValidationSecureServerCA.crt

    sudo cp filebeat-config/filebeat.yml /etc/filebeat/filebeat.yml


    echo "================================================"
    echo "Updating resource limits..."
    echo "================================================"
    # We use inotify to communicate between mandelboxes and the host service. We
    # use at least one per mandelbox, and Docker itself seems to use quite a few,
    # so we don't want this resource to be a limiting factor.
    sudo sh -c "echo 'fs.inotify.max_user_instances=2048' >> /etc/sysctl.conf" # default=128
    sudo sysctl -p

    echo "================================================"
    echo "Installing NVIDIA Persistence Daemon Unit..."
    echo "================================================"

    # Some research online indicates that this might be beneficial towards ensuring that our mandelboxes
    # start up more quickly. This is likely not a complete solution, but it might provide some nice benefits.
    # For more information, see the following link:
    # https://download.nvidia.com/XFree86/Linux-x86_64/396.51/README/nvidia-persistenced.html

    cat << EOF | sudo tee /etc/systemd/system/nvidia-persistenced.service > /dev/null
[Unit]
Description=NVIDIA Persistence Daemon
Wants=syslog.target
[Service]
Restart=yes
User=root
Type=forking
ExecStart=/usr/bin/nvidia-persistenced -V
ExecStopPost=/bin/rm -rf /var/run/nvidia-persistenced
[Install]
WantedBy=multi-user.target
EOF

    sudo /bin/systemctl daemon-reload
    sudo systemctl enable --now nvidia-persistenced.service
    echo "Enabled NVIDIA Persistence Daemon"
}

# `local_development_steps` contains the commands that are specific to setting
# up a local environment. This was formerly known as
# `setup_localdev_dependencies.sh`.
#
# Args: none
local_development_steps () {
    cd "$DIR/.."

    echo "================================================"
    echo "Installing Python dependencies..."
    echo "================================================"
    sudo apt-get install -y python3-pip
    find ./mandelbox-images -name 'requirements.txt' | sed 's/^/-r /g' | xargs sudo pip3 install
    cd host-setup

    echo "================================================"
    echo "Installing latest stable version of Go..."
    echo "================================================"
    sudo rm -rf /usr/local/go
    wget -qO - "https://dl.google.com/go/$(curl https://golang.org/VERSION?m=text).linux-amd64.tar.gz" | sudo tar -xz -C /usr/local
    echo 'export PATH=$PATH:/usr/local/go/bin' | sudo tee /etc/profile.d/02-golang-path
    source /etc/profile.d/02-golang-path

    echo "================================================"
    echo "Installing Heroku CLI..."
    echo "================================================"
    curl https://cli-assets.heroku.com/install-ubuntu.sh | sudo sh

    echo "================================================"
    echo "Creating symlink for root user to get AWS credentials..."
    echo "================================================"
    sudo ln -sf /home/ubuntu/.aws /root/.aws
}

# `deployment_setup_steps` contains the commands that are specific to setting
# up an environment for deploys (i.e. building an AMI). This was formerly known
# as `setup_ubuntu20_ami_host.sh`.
#
# Args: GH_USERNAME, GH_PAT, GIT_BRANCH, GIT_HASH, LOGZ_TOKEN
deployment_setup_steps() {
    cd "$DIR"

    # Parse input variables
    GH_USERNAME=${1}
    GH_PAT=${2}
    GIT_BRANCH=${3}
    GIT_HASH=${4}
    LOGZ_TOKEN=${5}

    # Set IP tables for routing networking from host to mandelboxes
    sudo sh -c "echo 'net.ipv4.conf.all.route_localnet = 1' >> /etc/sysctl.conf"
    sudo sysctl -p /etc/sysctl.conf
    echo iptables-persistent iptables-persistent/autosave_v4 boolean true | sudo debconf-set-selections
    echo iptables-persistent iptables-persistent/autosave_v6 boolean true | sudo debconf-set-selections
    sudo apt-get install -y iptables-persistent

    # Disable mandelboxes from accessing the instance metadata service on the host.
    # Critical to prevent IAM escalation from within mandelboxes while still using Docker.
    sudo iptables -I DOCKER-USER -i docker0 -d 169.254.169.254 -p tcp -m multiport --dports 80,443 -j DROP
    sudo iptables -I DOCKER-USER -i docker0 -d 169.254.170.2   -p tcp -m multiport --dports 80,443 -j DROP

    sudo sh -c 'iptables-save > /etc/iptables/rules.v4'

    # Remove a directory (this is necessary for the userdata to run for some reason)
    sudo rm -rf /var/lib/cloud/*

    # Also remove ECS directory, in case this is running on an ECS-optimized AMI
    sudo rm -f /var/lib/ecs/data/*

    # The Host Service gets built in the `fractal-build-and-deploy.yml` workflow and
    # uploaded from this Git repository to the AMI during Packer via ami_config.pkr.hcl
    # It gets enabled in base_userdata_template.sh

    # Here we pre-pull the desired mandelbox-images onto the AMI to speed up mandelbox startup.
    ghcr_uri=ghcr.io
    echo "$GH_PAT" | sudo docker login --username "$GH_USERNAME" --password-stdin "$ghcr_uri"
    pull_image_base="$ghcr_uri/fractal/$GIT_BRANCH/browsers/chrome"
    pull_image="$pull_image_base:$GIT_HASH"
    echo "pulling image: $pull_image"
    sudo docker pull "$pull_image"

    # Tag the image as `current-build` for now as well, so the client app can ask
    # for `current-build` without worrying about commit hash mismatches (yet).
    # Once maintenance mode removal is solidly implemented, we _shouldn't_ need it,
    # but I strongly suspect our deployment pipeline is not up to snuff for us to
    # actually get rid of this quite yet.
    sudo docker tag "$pull_image" "$pull_image_base:current-build"

    # Replace the dummy token in filebeat config with the correct token.
    # This will be done in GHA when the AMI for {dev,staging,prod} is being built through CI
    sudo sed -i "s/DUMMY_TOKEN_REPLACE_ME/$LOGZ_TOKEN/g" /etc/filebeat/filebeat.yml
    # Replace the default branch name to appropriate branch name parsed in GHA workflow.
    sudo sed -i "s/local-dev/$GIT_BRANCH/g" /etc/filebeat/filebeat.yml
    # Enable & Start the installed services for filebeat.
    sudo systemctl enable filebeat
    sudo systemctl start filebeat

    echo
    echo "Install complete. Make sure you do not reboot when creating the AMI (check NO REBOOT)"
    echo
}

# `common_steps_post` contains the commands that are to be run after
# common_steps()` and the environment-specific functions. This is the last
# function that runs in both environments.
#
# Args: none
common_steps_post () {
    cd "$DIR"

    echo "================================================"
    echo "Cleaning up the image a bit..."
    echo "================================================"

    sudo apt-get autoremove -y

    echo
    echo "Install complete. If you set this machine up for local development, please 'sudo reboot' before continuing."
    echo
    exit 0
}


# Parse arguments (derived from https://stackoverflow.com/a/7948533/2378475)
# I'd prefer not to have the short arguments at all, but it looks like getopt
# chokes without them.
TEMP=`getopt -o hld --long help,usage,localdevelopment,deployment -n 'setup_host.sh' -- "$@"`
eval set -- "$TEMP"

LOCAL_DEVELOPMENT=
DEPLOYMENT=
while true; do
    case "$1" in
        -h | --help | --usage) usage ;;
        -l | --localdevelopment ) LOCAL_DEVELOPMENT=true; shift ;;
        -d | --deployment ) DEPLOYMENT=true; shift ;;
        -- ) shift; break ;;
        * ) echo "We should never be able to get into this argument case! Unknown argument passed in: $1"; exit -1 ;;
    esac
done

if [[ -z "$LOCAL_DEVELOPMENT" && -z "$DEPLOYMENT" ]]; then
    usage
fi

if [[ -n "$LOCAL_DEVELOPMENT" && -n "$DEPLOYMENT" ]]; then
    echo 'Both `--localdevelopment` and `--deployment` were passed in. Make up your mind!'
    exit -1
fi

if [[ -n "$LOCAL_DEVELOPMENT" ]]; then
    printf "Setting up host for local development...\n\n"

    common_steps
    local_development_steps
    common_steps_post
fi

if [[ -n "$DEPLOYMENT" ]]; then
    printf "Setting up host for deployment...\n\n"

    common_steps
    deployment_setup_steps "$@"
    common_steps_post
fi

echo "Should never get here!"
exit 1
