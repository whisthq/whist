FROM python

COPY ./requirements.txt /root/monorepo-config/requirements.txt

RUN pip install -r /root/monorepo-config/requirements.txt

COPY . /root/monorepo-config

ENTRYPOINT ["python", "/root/monorepo-config/main.py", "config"]
