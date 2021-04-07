# Load Testing

Load testing is a valuable tool for discovering complex bugs in our code base. Previously, it has helped identify issues with our locking mechanisms on webserver and race conditions in ECS-host. It runs every Sunday at 2am EST in the `main-webserver-load-testing` workflow. Load testing source is a little complex to navigate, so this README exists to help demystify all that is going on. First, the files:

```
./scripts/load_testing
├── __init__.py # packages load_testing module
├── load_test_utils.py # load test utility methods/constants
├── load_test_management.py # helps set up and tear down a load test
├── load_test_driver.py # overarching function to actually run a local load test
├── requirements.txt # requirements for running the load test scripts specifically
```

## Testing Against Staging

See the workflow at `.github/workflows/main-webserver-load-testing.yml` (specifically the "Setup and run load test" step).
Those are the steps you'd need to take to setup, run, and cleanup a load test. Just open a Python shell and follow along, or make a Python file that runs as many of the commands as you'd like in one go.

Note 1: You can run `run_local_load_test` locally.

Note 2: To run against dev, you may need to run `load_test_management.py:make_load_test_user` first. Check if they exist in the dbs.

## From Scratch in a Review App

1. Make a review app. See [here](https://www.notion.so/tryfractal/Steps-for-Doing-a-Webserver-Review-App-823cadbb422e401087625c69172cf4fb) for instructions.
2. Add hirefire to review app. See the hirefire [dashboard](https://manager.hirefire.io/). You'll need to configure it for the review app. Copy the configurations for dev. You should get a command of the form `heroku config:add -a <HEROKU_REVIEW_APP_NAME> HIREFIRE_TOKEN=<HIREFIRE_TOKEN>` to run.
3. Run `load_test_management.py:make_load_test_user` for however many concurrent users you plan on doing.
4. Follow the steps for testing against `staging`.
5. For cleanup, remember to delete the Hirefire application. Of course, also remember to do the usual cleanup steps shown in the workflow.

## Load Testing Results

See https://docs.google.com/spreadsheets/u/2/d/1yGook9y4vc6leG_uGub8ft-AN9T7ilYvbxrcRPXZqSY/edit#gid=0.
