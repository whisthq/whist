FROM python

COPY ./requirements.txt .

RUN pip install -r requirements.txt

ENTRYPOINT ["python", ".github/actions/monorepo-config/main.py"]
