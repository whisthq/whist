FROM ubuntu:20.04

# Install python
RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-dev build-essential libssl-dev libffi-dev  && \
    apt-get clean

# Upgrade python pip
RUN pip3 install --upgrade pip setuptools wheel && \
    ln -s /usr/bin/python3 /usr/bin/python && \
    ln -s /usr/bin/pip3 /usr/bin/pip

# Adds user and home folder
RUN useradd -ms /bin/bash fractal

# Installs python requirements
ADD requirements.txt /home/fractal/requirements.txt
RUN pip3 install -r /home/fractal/requirements.txt

# Installs curl
RUN apt-get install -y vim curl

# Install heroku cli
RUN curl https://cli-assets.heroku.com/install-ubuntu.sh | sh

RUN python --version

USER fractal
WORKDIR /home/fractal

COPY --chown=fractal . .

CMD /bin/bash
ENTRYPOINT [ "/home/fractal/entrypoint.sh" ]
