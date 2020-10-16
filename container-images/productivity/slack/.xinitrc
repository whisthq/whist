#!/usr/bin/env bash

i3 &

# exec gimp
exec runuser --login fractal -c \
        'DISPLAY=:10 \
        LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
        LOCAL=yes \
        LC_ALL=C \
        PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
        DEBIAN_FRONTEND=noninteractive \
        /usr/lib/slack/slack'
