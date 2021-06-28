FROM python:3.9.5

COPY ./requirements.txt /root/monorepo-config/requirements.txt

RUN pip install -r /root/monorepo-config/requirements.txt

COPY . /root/monorepo-config
