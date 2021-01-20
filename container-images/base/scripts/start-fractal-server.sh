#!/bin/sh

# This script starts the Fractal protocol server, and assumes that it is
# being run within a Fractal Docker container.

# Set/Retrieve Container parameters

# TODO: symlink the appropriate folders from the downloaded config, if it exists
#       perhaps better is to copy the contents to the appropriate locations in the
#       host service from the get-go, instead of symlinking ahead of tim
#       early symlinks to things that eventually won't exist may lead to broken links
# - actually, cannot copy-paste directly because then can't upload back to S3 from the host
# - perhaps create the
# all configs are at /home/fractal/...
# programs that have a /home/fractal/.nv folder for cache are marked with (nv)
# some programs might have stuff in the .local/share folder ?
# it is also possible that each app needs the config of its dependencies also installed
# brave: .config/BraveSoftware/BraveBrowser
# chrome: .config/google-chrome
# firefox: .mozilla/firefox
# sidekick: .config/sidekick
# ____
# blender: .config/blender (nv)
# blockbench: .config/Blockbench
# darktable: .config/darktable (nv)
# figma: .config/Figma
# gimp: .config/GIMP
# godot: .config/godot (nv)
# inkscape: .config/inkscape
# kdenlive: .config/kdenlive-layoutsrc, .config/kdenliverc (nv)
# krita: .config/kritadisplayrc, .config/kritarc
# lightworks: Lightworks (nv)
# shotcut: .config/Meltytech/Shotcut.conf (nv)
# texturelab: .config/texturelab (nv)
# ____
# discord: .config/discord
# freecad: .config/FreeCAD
# notion: .config/Notion
# rstudio: .rstudio-desktop, .config/rstudio (nv)
# slack: .config/Slack (nv)

CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
FRACTAL_MAPPINGS_DIR=/fractal/containerResourceMappings
IDENTIFIER_FILENAME=hostPort_for_my_32262_tcp
PRIVATE_KEY_FILENAME=/usr/share/fractal/private/aes_key
WEBSERVER_URL_FILENAME=/usr/share/fractal/private/webserver_url
SENTRY_ENV_FILENAME=/usr/share/fractal/private/sentry_env

# Define a string-format identifier for this container
IDENTIFIER=$(cat $FRACTAL_MAPPINGS_DIR/$IDENTIFIER_FILENAME)

# Create list of command-line arguments to pass to the Fractal protocol server
OPTIONS=""

# Send in AES private key, if set
if [ -f "$PRIVATE_KEY_FILENAME" ]; then
    export FRACTAL_AES_KEY=$(cat $PRIVATE_KEY_FILENAME)
    OPTIONS="$OPTIONS --private-key=$FRACTAL_AES_KEY"
fi

# Send in Fractal webserver url, if set
if [ -f "$WEBSERVER_URL_FILENAME" ]; then
    export WEBSERVER_URL=$(cat $WEBSERVER_URL_FILENAME)
fi
OPTIONS="$OPTIONS --webserver=$WEBSERVER_URL"

# Send in Sentry environment, if set
if [ -f "$SENTRY_ENV_FILENAME" ]; then
    export SENTRY_ENV=$(cat $SENTRY_ENV_FILENAME)
    OPTIONS="$OPTIONS --environment=$SENTRY_ENV"
fi

# Create a google_drive folder in the user's home
ln -sf /fractal/cloudStorage/$IDENTIFIER/google_drive /home/fractal/

# Check whether the '.exists' file is in the user app config folder
# If no, then copy default configs to the synced app config folder
existsFile=/fractal/userConfig/$IDENTIFIER/.exists
if [ ! -f "$existsFile" ]; then
    cp -rT /home/fractal/.config/google-chrome /fractal/userConfig/$IDENTIFIER/google-chrome
    touch $existsFile
fi

# Create symlinks between all local configs and the target locations for the running application
cp -rflT /fractal/userConfig/$IDENTIFIER/google-chrome /home/fractal/.config/google-chrome

# Send in identifier
OPTIONS="$OPTIONS --identifier=$IDENTIFIER"

/usr/share/fractal/FractalServer $OPTIONS

# Send logs to webserver
curl \
    --header "Content-Type: application/json" \
    --request POST \
    --data @- \
    $WEBSERVER_URL/logs \
    <<END
{
    "sender": "server",
    "identifier": "$IDENTIFIER",
    "secret_key": "$FRACTAL_AES_KEY",
    "logs": $(jq -Rs . </usr/share/fractal/log.txt)
}
END
