# Load Testing

Load testing is a valuable tool for discovering complex bugs in our code base. Previously, it has helped identify issues with our locking mechanisms on webserver and race conditions in ECS-host. It runs every Sunday at 2am EST in the `main-webserver-load-testing` workflow. Load testing source is a little complex to navigate, so this README exists to help demystify all that is going on. First, the files:

```
./scripts/load_testing
├── scripts/ # contains symlinked files from the scripts in webserver root; reduces code copying but allows us to build the lambda function easily
├── __init__.py # packages load_testing module
├── lambda_function.py # runs as an AWS lambda function. We use this to stress our webserver with many concurrent requests
├── load_test_analysis.py # analyzes a distributed (AWS lambda) or local test and optionally saves to a db for persistence
├── load_test_core.py # core load test functions, shared by a local and distributed load test
├── load_test_management.py # helps set up and tear down a load test
├── load_test_models.py # SQLAlchemy db models to save a load test's results
├── load_test.py # overarching functions to actually run a local or distributed load test
├── Makefile # packages and uploads the AWS lambda function
├── requirements.txt # requirements for ONLY the lambda function
```

The overall strategy is to share as much of the load testing code as possible in `load_test_core.py`. It was a priority to have both local (your machine vs server) and distributed (many machines vs server) load tests. We chose AWS lambda functions to run distributed load tests. While simple, a key limitation of AWS lambda is that all dependencies (including 3rd party) must come included in the packaged zip file. We can't use too many 3rd party libraries (like [locust](https://locust.io/)) because that blows up package size. `load_test_core.py` essentially implements a minimal load testing framework.

You might be wondering why we have a `scripts` folder in this directory that shares some files (`__init__.py`, `celery_scripts.py`, `utils.py`) with the `scripts` folder in webserver root. These are actually symlinked references to the files in `main-webserver/scripts`. Changes in one file will show up in the other. Note: the symlinks are relative, so changing the location of the original file or the symlink will break the symlink. The main reason we can't just do this:

```python
from scripts.celery_scripts poll_celery_task
```

in our load testing code is because the AWS lambda function builds in the current directory. Even if we told the `Makefile` to package `celery_scripts.py` from `main-webserver/scripts`, we'd need to dynamically modify all load testing scripts to import from `celery_scripts` instead of from `scripts.celery_scripts.py`. The current solution is to include the symlinked `./scripts` directory. The Makefile packages specific files in the current directory and `./scripts` directory when building the AWS lambda function. In AWS lambda contexts, the code is at `./scripts/celery_scripts.py`. In local contexts, the code is at `main-webserver/scripts/celery_scripts.py`. Hence, importing from `scripts.celery_scripts` works in both contexts.

## Testing Against Staging

See the workflow at `.github/workflows/main-webserver-load-testing.yml` (specifically the "Setup and run load test" step).
Those are the steps you'd need to take to setup, run, and cleanup a load test. Just open a Python shell and follow along, or make a Python file that runs as many of the commands as you'd like in one go.

Note 1: You can run `run_local_load_test` or `run_distributed_load_test` locally.

Note 2: To run against dev/prod, you need to run `load_test_management.py:make_load_test_user` first. You may
also need to make the `analytics` schema and `load_testing` table, if it does not already exist.

## From Scratch in a Review App

1. Make a review app. See [here](https://www.notion.so/tryfractal/Steps-for-Doing-a-Webserver-Review-App-823cadbb422e401087625c69172cf4fb) for instructions.
2. Add hirefire to review app. See the hirefire [dashboard](https://manager.hirefire.io/). You'll need to configure it for the review app. Copy the configurations for dev. You should get a command of the form `heroku config:add -a <HEROKU_REVIEW_APP_NAME> HIREFIRE_TOKEN=<HIREFIRE_TOKEN>` to run.
3. Run `load_test_management.py:make_load_test_user` for however many concurrent requests you plan on doing.
4. Follow the steps for testing against `staging`. Don't save the load testing results to a table anywhere, because this review app won't persist and this is presumably just for your own testing.
5. For cleanup, remember to delete the Hirefire application. Of course, also remember to do the usual cleanup steps shown in the workflow.

## Load Testing Results

See https://docs.google.com/spreadsheets/u/2/d/1yGook9y4vc6leG_uGub8ft-AN9T7ilYvbxrcRPXZqSY/edit#gid=0.
