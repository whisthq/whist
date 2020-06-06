FROM ubuntu:20.04

RUN apt-get update && apt-get clean && apt-get install -y \
    x11vnc \
    wmctrl \
    wget \
    gnupg2 \
    libgbm1 \
    libxcb-dri3-0 \
    wget \
    python \
    python3 \
    gnupg2 \
    sudo \
    vim \
    apt-utils \
    && wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
    && echo "deb http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google.list \
    && apt-get update && apt-get -y install google-chrome-stable \
    && apt-get clean


RUN wget -q -O utils.sh https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/utils2.sh >>/dev/null 2>&1

# Avoids interactive installation for tzdata
# Uses UTC
ENV DEBIAN_FRONTEND="noninteractive"
ENV LOCAL="yes"
RUN apt-get -y install tzdata keyboard-configuration ufw

# Workaround for sudo bug in containers https://github.com/sudo-project/sudo/issues/42
RUN echo "Set disable_coredump false" >> /etc/sudo.conf

# install virtual display 
RUN /bin/bash -c "source utils.sh \ 
    && Install-VirtualDisplay"

RUN useradd --create-home fractal \
    && usermod -aG sudo fractal \ 
    && echo 'fractal:password1234567.' | chpasswd

# Install all required components for Fractal to perform well
# Split into multiple steps for caching
RUN /bin/bash -c "source utils.sh \ 
    && Update-Linux \
    && Set-Time \
    && Install-VirtualDisplay"
RUN /bin/bash -c "source utils.sh \ 
    && Install-7Zip \
    && Install-Curl \
    && Install-GoogleChrome"
RUN /bin/bash -c "source utils.sh \ 
    && Install-NvidiaTeslaPublicDrivers \
    && Disable-TCC \
    && Set-OptimalGPUSettings"
RUN /bin/bash -c "source utils.sh \ 
    && Install-CustomGDMConfiguration \
    && Install-CustomX11Configuration"
RUN /bin/bash -c "source utils.sh \ 
    && Set-FractalDirectory \
    && Install-FractalServer master \
    && Install-FractalService"
RUN /bin/bash -c "source utils.sh \ 
    && Install-FractalExitScript \
    && Install-FractalAutoUpdate master \
    && Install-FractalLinuxInputDriver \
    && Install-FractalWallpaper \
    && Enable-FractalFirewallRule"
RUN /bin/bash -c "source utils.sh \ 
    && Install-Unison \
    && Enable-SSHKey \
    && Disable-Shutdown \
    && Add-AutoLogin \ 
    && Disable-AutomaticLockScreen "



ADD bootstrap.sh /home/fractal


RUN chmod +x /home/fractal/bootstrap.sh \
    && chown -v -R fractal:fractal /home/fractal

USER fractal

# ENTRYPOINT ["google-chrome"]
CMD '/home/fractal/bootstrap.sh'
