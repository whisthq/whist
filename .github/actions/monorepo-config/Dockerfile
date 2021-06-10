FROM python

COPY ./requirements.txt /monorepo-config/requirements.txt

RUN pip install -r /monorepo-config/requirements.txt

COPY . /monorepo-config

ENTRYPOINT ["python", ".github/actions/monorepo-config/main.py", "config"]
