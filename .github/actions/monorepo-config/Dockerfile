FROM python

COPY ./requirements.txt .

RUN pip install -r requirements.txt

# commenting

ENTRYPOINT ["python", ".github/actions/monorepo-config/main.py"]
