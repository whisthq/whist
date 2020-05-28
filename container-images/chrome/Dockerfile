# FROM ubuntu:18.04

# # general container preparation and installation of Chrome
# RUN apt-get update && apt-get clean && apt-get install -y \
#     wget \
#     python \
#     python3 \
#     gnupg2 \
#     sudo

# # Installs google-chrome-stable
# RUN wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
#     && echo "deb http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google.list \
#     && apt-get update && apt-get -y install google-chrome-stable

# # fetch our bash script utils
# RUN wget utils.sh https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/utils.sh >>/dev/null 2>&1; echo $

# # Avoids interactive installation for tzdata
# # Uses UTC
# RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata



# # install virtual display 
# RUN /bin/bash -c "source ./utils.sh \ 
#     && Install-VirtualDisplay && reboot"


# # install all required components for Fractal to perform well
# RUN /bin/bash -c "source ./utils.sh \ 
#     && Update-Linux \
#     Set-Time \
#     Install-7Zip \
#     Install-Curl \
#     Install-CustomGDMConfiguration \
#     Install-CustomX11Configuration \
#     Set-FractalDirectory \
#     Install-FractalServer \
#     Install-ProcessManager \
#     Install-FractalExitScript \
#     Install-FractalAutoUpdate \
#     Install-FractalLinuxInputDriver \
#     Install-FractalWallpaper \
#     Enable-FractalFirewallRule \
#     Install-Unison \
#     Enable-SSHKey \
#     Disable-Shutdown \
#     Add-AutoLogin"

# # create user
# RUN useradd fractal \
#     && mkdir -p /home/fractal \
#     && chown -v -R fractal:fractal /home/fractal

# # clean up and reboot for use
# RUN sudo rm -f "utils.sh" && reboot
# FROM debian:bullseye-slim
# LABEL maintainer "Jessie Frazelle <jess@linux.com>"

# # Install Chrome
# RUN apt-get update && apt-get install -y \
# 	apt-transport-https \
# 	ca-certificates \
# 	curl \
# 	gnupg \
# 	hicolor-icon-theme \
# 	libcanberra-gtk* \
# 	libgl1-mesa-dri \
# 	libgl1-mesa-glx \
# 	libpangox-1.0-0 \
# 	libpulse0 \
# 	libv4l-0 \
# 	fonts-symbola \
# 	--no-install-recommends \
# 	&& curl -sSL https://dl.google.com/linux/linux_signing_key.pub | apt-key add - \
# 	&& echo "deb [arch=amd64] https://dl.google.com/linux/chrome/deb/ stable main" > /etc/apt/sources.list.d/google.list \
# 	&& apt-get update && apt-get install -y \
# 	google-chrome-stable \
# 	--no-install-recommends \
# 	&& apt-get purge --auto-remove -y curl \
# 	&& rm -rf /var/lib/apt/lists/*

# # Add chrome user
# RUN groupadd -r chrome && useradd -r -g chrome -G audio,video chrome \
#     && mkdir -p /home/chrome/Downloads && chown -R chrome:chrome /home/chrome

# # COPY local.conf /etc/fonts/local.conf

# # Run Chrome as non privileged user
# USER chrome

# # Autorun chrome
# ENTRYPOINT [ "google-chrome" ]
# CMD [ "--user-data-dir=/data" ]


FROM ubuntu:20.04

RUN apt-get update && apt-get clean && apt-get install -y \
    x11vnc \
    xvfb \
    fluxbox \
    wmctrl \
    wget \
    gnupg2 \
    libgbm1 \
    libxcb-dri3-0 \
    && wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
    && echo "deb http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google.list \
    && apt-get update && apt-get -y install google-chrome-stable


RUN useradd fractal \
    && mkdir -p /home/fractal \
    && chown -v -R fractal:fractal /home/fractal

ADD bootstrap.sh /home/fractal


RUN chmod +x /home/fractal/bootstrap.sh \
    && chown -v -R fractal:fractal /home/fractal

USER fractal

# ENTRYPOINT ["google-chrome"]
CMD '/home/fractal/bootstrap.sh'
